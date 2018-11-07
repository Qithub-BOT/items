<?php
namespace Qithub\QiitaCache;

/* [Functions] ============================================================== */

/* ---------------------------------------------------------------------- [C] */

function copyCacheToSpams($json)
{
    if (empty($json)) {
        return false;
    }

    $id_item        = getItemIdGiven();
    $path_file_spam = getPathFileSpam($id_item);

    if (file_exists($path_file_spam)) {
        return true;
    }

    $array = json_decode($json, JSON_OBJECT_AS_ARRAY);
    $user  = \getValue('user', $array, []);

    $result['id_item']   = \getValue('id', $array);
    $result['id_user']   = \getValue('id', $user);
    $result['url_cache'] = URL_ENDPOINT_BASE  . '?id=' . $id_item;
    $result['url_raw']   = URL_ENDPOINT_QIITA . $id_item;
    $result['date_post'] = \getValue('created_at', $array);

    $json = json_encode($result, \JSON_COMMON_OPTION);

    return \putContentsToFile($path_file_spam, $json);
}

/* ---------------------------------------------------------------------- [E] */

function echoJson($json_string, $is_spam = NOT_SPAM, $options = \JSON_COMMON_OPTION)
{
    if (! \isValidJson($json_string)) {
        \dieMsg('Invalid JSON given.(JSON empty)', __LINE__);
    }

    $array = json_decode($json_string, JSON_OBJECT_AS_ARRAY);

    $array['is_spam'] = (IS_SPAM === $is_spam);

    $json_string = json_encode($array, $options);
    $etag        = hash('md5', $json_string);

    if (! \isCli() && ! headers_sent()) {
        header('Content-type: application/json; charset=utf-8');
        header('Content-Length: '.strlen($json_string));
        header('Etag: ' . $etag);
    }

    echo $json_string;
}

function echoTagsAsCommonFormat($tags_search, $return = RETURN_AS_ARRAY)
{
    $result = [];

    if (is_string($tags_search)) {
        $tags_search[] = $tags_search;
    }

    foreach ($tags_search as $tag_search) {
        $tag_search = stripTagAsKey($tag_search);
        $tag_search = getTagAsCommonFormat($tag_search);
        if (! empty($tag_search)) {
            $result[] = $tag_search;
        }
    }

    if (RETURN_AS_JSON === $return) {
        return echoJson(json_encode($result));
    }

    return $result;
}

/* ---------------------------------------------------------------------- [G] */

function getItemIdGiven()
{
    static $result;

    if (isset($result)) {
        return $result;
    }

    $id_item = hasItemIdInArgs(RETURN_AS_ID);
    if (! empty($id_item)) {
        $result = $id_item;
        return $result;
    }

    $id_item = hasItemIdInGet(RETURN_AS_ID);
    if (! empty($id_item)) {
        $result = $id_item;
        return $result;
    }

    return false;
}

function getJsonItemFromApi($id_item)
{
    if (empty($id_item)) {
        return false;
    }

    $url  = URL_ENDPOINT_QIITA . $id_item;
    $json = \getContentsFromUrl($url);

    if (null === json_decode($json)) {
        return false;
    }

    return $json;
}

function getNameTagGiven($return = RETURN_AS_ARRAY)
{
    $name_tag = hasTagsInArgs($return);
    if (! empty($name_tag)) {
        return $name_tag;
    }

    $name_tag = hasTagsInGet($return);
    if (! empty($name_tag)) {
        return $name_tag;
    }

    return false;
}

function getPathDirCache(array $settings)
{
    $path_dir_curr  = \getValue('path_dir_curr', $settings);
    $name_dir_cache = \getValue('name_dir_cache', $settings);
    $path_dir_cache = $path_dir_curr . \DIR_SEP . $name_dir_cache;

    return $path_dir_cache;
}

function getPathDirSpam(array $settings)
{
    return \getValue('path_dir_spams', $settings, false);
}

function getPathFileCache($id_item)
{
    global $settings;

    $path_dir_cache = getPathDirCache($settings);

    if (! is_dir($path_dir_cache)) {
        return false;
    }

    // Qiita記事IDの最初の１文字をカテゴリ名としてディレクトリにする
    // https://github.com/Qithub-BOT/Qithub-ORG/issues/152
    $name_dir_group = $id_item[0];
    $path_dir_cache = $path_dir_cache . \DIR_SEP . $name_dir_group;

    if (! is_dir($path_dir_cache) && ! mkdir($path_dir_cache, 0700)) {
        \dieMsg('Error: Can not create spam cache directory.', __LINE__);
    }

    $name_file_cache = $id_item . EXT_FILE_CACHE;

    return $path_dir_cache . \DIR_SEP . $name_file_cache;
}

function getPathFileSpam($id_item)
{
    global $settings;

    if (! \isValidFormatId('item_qiita', $id_item)) {
        return false;
    }

    $path_dir_spams = getPathDirSpam($settings);

    if (! is_dir($path_dir_spams)) {
        return false;
    }

    // Qiita記事IDの最初の１文字をカテゴリ名としてディレクトリにする
    // https://github.com/Qithub-BOT/Qithub-ORG/issues/152
    $name_dir_group = $id_item[0];
    $path_dir_spams = $path_dir_spams . \DIR_SEP . $name_dir_group;

    if (! is_dir($path_dir_spams) && ! mkdir($path_dir_spams, 0700)) {
        \dieMsg('Error: Can not create spam cache directory.', __LINE__);
    }

    $name_file_spam = $id_item . EXT_FILE_CACHE;

    return $path_dir_spams . \DIR_SEP . $name_file_spam;
}

function getPathFileTags(array $settings = [])
{
    if (empty($settings)) {
        global $settings;
    }

    $path_dir_curr  = \getValue('path_dir_curr', $settings, '.');
    $name_file_tags = \getValue('name_file_tags', $settings);
    $path_file_tags = $path_dir_curr . \DIR_SEP . $name_file_tags;

    return $path_file_tags;
}

function getTagAsCommonFormat($name_tag)
{
    static $list_tags;

    if (! isset($list_tags)) {
        // 出現タグ一覧の再利用に static 変数に代入
        $list_tags = getTagsFromFileAsArray(getPathFileTags());
        //$list_tags = getContentsFromFile(getPathFileTags());
    }

    $key = stripTagAsKey($name_tag);

    // 初出現の場合は同じタグを返す
    if (! isset($list_tags[$key])) {
        return $name_tag;
    }

    // 該当タグの出現リストから最大出現数のタグを返す
    $array = $list_tags[$key];
    $keys  = array_keys($array, max($array));

    ksort($keys, SORT_NATURAL);

    return $keys[0];
}

function getTagsFromFileAsArray($path_file_tags)
{
    $json   = \getContentsFromFile($path_file_tags);
    $result = json_decode($json, JSON_OBJECT_AS_ARRAY);

    if (NO_CONTEXT === $result) {
        $result = [];
    }

    return $result;
}

function getUrlOfItem($id_item)
{
    if (empty($id_item)) {
        return false;
    }

    $url    = "https://qiita.com/items/{$id_item}";
    $status = getStatusCodeFromUrl($url);

    switch ($status) {
        case 404:
            return false;
        case 301: // リダイレクト
            return getUrlToRedirect($url);
        case 200:
            return $url;
        default:
            return false;
    }
}

/* ---------------------------------------------------------------------- [H] */

function hasItemIdGiven()
{
    return hasItemIdInArgs() || hasItemIdInGet();
}

function hasItemIdInArgs($return = RETURN_AS_BOOL)
{
    global $argv;

    if (! isset($argv)) {
        return false;
    }

    if (false === strpos(\getValue(1, $argv), '-id')) {
        return false;
    }

    $id_item = \getValue(2, $argv);

    if (! isValidFormatId('item_qiita', $id_item)) {
        return false;
    }

    return (RETURN_AS_BOOL === $return) ?: $id_item;
}

function hasItemIdInGet($return = RETURN_AS_BOOL)
{
    global $_GET;

    $id_item = \getValue('id', $_GET);

    if (! isValidFormatId('item_qiita', $id_item)) {
        return false;
    }

    return (RETURN_AS_BOOL === $return) ?: $id_item;
}

function hasTagsInArgs($return = RETURN_AS_BOOL)
{
    global $argv;

    if (! isset($argv)) {
        return false;
    }

    if (false === strpos(\getValue(1, $argv), '-tag')) {
        return false;
    }

    $tags = \getValue(2, $argv);
    $tags = str_replace(' ', ',', trim($tags));

    if (RETURN_AS_STRING === $return) {
        return $tags;
    }

    if (RETURN_AS_ARRAY === $return) {
        if (empty($tags)) {
            return [];
        }

        return explode(',', $tags);
    }

    if (RETURN_AS_JSON === $return) {
        $array = (empty($tags)) ? [] : explode(',', $tags);
        return json_encode($array, JSON_COMMON_OPTION);
    }

    return ! empty($tags);
}

function hasTagsInGet($return = RETURN_AS_BOOL)
{
    global $_GET;

    $tags = \getValue('tag', $_GET);
    $tags = str_replace(' ', ',', trim($tags));

    if (RETURN_AS_STRING === $return) {
        return $tags;
    }

    if (RETURN_AS_ARRAY === $return) {
        if (empty($tags)) {
            return [];
        }

        return explode(',', $tags);
    }

    if (RETURN_AS_JSON === $return) {
        $array = (empty($tags)) ? [] : explode(',', $tags);
        return json_encode($array, JSON_COMMON_OPTION);
    }

    return ! empty($tags);
}

/* ---------------------------------------------------------------------- [I] */

function initializeDirectories($settings)
{
    // キャッシュ・ディレクトリ確認/作成
    $path_dir_cache = getPathDirCache($settings);
    if (! is_dir($path_dir_cache) && ! mkdir($path_dir_cache, 0700)) {
        \dieMsg('Error: Can not create cache directory.', __LINE__);
    }

    // スパム・ディレクトリ確認/作成
    $path_dir_spams = getPathDirSpam($settings);
    if (! is_dir($path_dir_spams) && ! mkdir($path_dir_spams, 0700)) {
        \dieMsg('Error: Can not create spam cache directory.', __LINE__);
    }
}

function isInSpams($id_item)
{
    return file_exists(getPathFileSpam($id_item));
}

function isItem404($id_item)
{
    $url = getUrlOfItem($id_item);

    return false === $url;
}

function isUser404($id_user)
{
    $url = "https://qiita.com/{$id_user}";

    return 404 === \getStatusCodeFromUrl($url);
}

function isUserOfItem404($id_item)
{
    $url    = "https://qiita.com/api/v2/items/{$id_item}";
    $json   = \getContentsFromUrl($url);
    $object = json_decode($json);

    if (! isset($object->user->id)) {
        return null;
    }

    return isUser404($object->user->id);
}

/* ---------------------------------------------------------------------- [P] */

function putTagsToFile($path_file_tags, $json)
{
    if (! is_string($json)) {
        return false;
    }

    $array = json_decode($json, JSON_OBJECT_AS_ARRAY);
    if (JSON_DECODE_FAIL === $array) {
        return false;
    }

    $tags = \getValue('tags', $array, []);
    if (empty($tags)) {
        return false;
    }

    // 既存タグの読み込み
    $result = getTagsFromFileAsArray($path_file_tags);

    // タグの更新
    foreach ($tags as $tag) {
        $name_tag = $tag['name'];
        $key      = stripTagAsKey($name_tag);
        $name_tag = getTagAsCommonFormat($name_tag);
        $count    = 1;
        if (isset($result[$key][$name_tag])) {
            $count = ++$result[$key][$name_tag];
        }
        $result[$key][$name_tag] = $count;
    }

    // タグの保存
    $result_json = json_encode($result);

    return \putContentsToFile($path_file_tags, $result_json);
}

/* ---------------------------------------------------------------------- [S] */

function stripTagAsKey($name_tag)
{
    $name_tag = (string) $name_tag;
    $name_tag = stripItemTagMinimum($name_tag);
    return strtolower($name_tag);
}


function stripItemTagMinimum($name_tag)
{
    $name_tag = trim($name_tag);
    $name_tag = str_replace(' ', '_', $name_tag);
    $name_tag = str_replace('#', '', $name_tag);

    return htmlspecialchars($name_tag);
}