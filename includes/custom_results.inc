<?php

/**
 * @file
 * Class override for upei's islandora_scholar implementation
 */

/**
 * Extension of IslandoraSolrResults to create an alternative display type.
 */
class IslandoraSolrResultsCustom extends IslandoraSolrResults {

  /**
   * Based on the default solr results
   *
   * @see IslandoraSolrResults::displayResults()
   *
   * @param array $solr_results
   *   The processed Solr results from
   *   IslandoraSolrQueryProcessor::islandoraSolrResult.
   *
   * @return string
   *   Rendered Solr results.
   */
  public function printResults($solr_results) {
    self::createCoinsUrlForResults($solr_results);
    $this->customizeResults($solr_results);
    $solr_results = islandora_solr_prepare_solr_results($solr_results);
    $object_results = $solr_results['response']['objects'];
    $object_results = islandora_solr_prepare_solr_doc($object_results);
    self::replaceCommaWithSemicolon($object_results);
    $mod_path = drupal_get_path('module', 'islandora_solr_config');
    drupal_add_css("$mod_path/css/islandora_solr_config.theme.css");

    $elements = [];
    $elements['solr_total'] = $solr_results['response']['numFound'];
    $elements['solr_start'] = $solr_results['response']['start'];


    return theme('upei_roblib_scholar_search_custom', [
      'results' => $object_results,
      'elements' => $elements,
    ]);
  }

  /**
   * Adds and updates fields for UPEI customizations.
   *
   * Currently checks the list of datastreams from solr to see if PDF exists to
   * create the full text, turns dois into links, adds the bookmark form to
   * each result etc.
   *
   * @param array $solr_result
   */
  function customizeResults(&$solr_result) {
    // TODO We may want to make the base_url configurable in case we ever change resolvers.
    $results = &$solr_result['response']['objects'];
    foreach ($results as &$result) {
      if (in_array('PDF', $result['datastreams'])) {
        $options['html'] = TRUE;
        $options['attributes']['title'] = 'Full Text';
        $result['full_text_url'] = l(t("Full Text"), '/islandora/object/' .
          $result['PID'] . '/datastream/PDF/download', $options);
      }
      else {
        $result['full_text_url'] = '';
      }
      $id_for_form = upei_roblib_scholar_search_escape_pid($result['PID']);
      // we need to make the form ids unique, we then use hook_forms to map them to a real form id (because the form appears muliple times on one page).
      $bookmark_form = drupal_get_form('upei_roblib_scholar_search_bookmark_form_' . $id_for_form, $result['PID']);
      $result['roblib_bookmark_form'] = drupal_render($bookmark_form);
      if (!empty($result['solr_doc']['mods_identifier_doi_s'])) {
        $doi = $result['solr_doc']['mods_identifier_doi_s'];
        $result['solr_doc']['mods_identifier_doi_s'] =
          l($doi, DOI_BASE_URL . $doi);
      }
    }
  }

  /**
   * Replace the comma separator between authors, advisors, creators etc with a
   * semi colon.
   *
   * This method may fail on records with authors that only have one name
   * (missing lastname or missing firstname).  Names that have a comma in them
   * may also cause issues.
   *
   * @param array $results
   */
  static function replaceCommaWithSemicolon(&$results) {
    foreach ($results as &$result) {
      foreach ($result['solr_doc'] as $key => &$field) {
        if (strpos($key, '_lastname_first_')) {
          $field['value'] = preg_replace('/(.+?),(.+?),/', '$1,$2;', $field['value']);
          // we have mods that have multiple empty advisor fields
          if (trim($field['value']) === ',') {
            $field['value'] = '';
          }
        }
      }
    }
  }


  /**
   * Creates a url based on coins specification.
   *
   * We could use the citeproc converter to grab this data from mods but it
   * will be expensive as it will involve a round trip to Fedora for every
   * search result.  If we need to generate a citation for each search result
   * we should use citeproc.  If not we should try to get the relevant
   * information from the solr results (for now using solr results)
   *
   * @param $pid
   */
  static function createCoinsUrlForResults(&$solr_result) {
    // TODO We may want to make the base_url configurable in case we ever change resolvers.

    $coins_base_url = 'http://resolver.ebscohost.com/openurl';
    $results = &$solr_result['response']['objects'];
    $img_variables = [
      'path' => 'http://library.upei.ca/sites/all/files/sharedfolder/files/getit_upei.gif',
      'alt' => 'Get it at UPEI',
      'title' => 'Get it at UPEI',
      'attributes' => ['class' => 'coins-img',],
    ];
    $img = theme('image', $img_variables);
    foreach ($results as &$result) {
      $genre = empty($result['solr_doc']['mods_genre_s']) ? NULL : $result['solr_doc']['mods_genre_s'];
      if (!empty($genre) && !(in_array('PDF', $result['datastreams']))) {
        if (stripos((string) $genre, 'book') !== FALSE || stripos((string) $genre, 'conference') !== FALSE) {
          $options = self::createCoinsUrlForBookOrConference($result);
        }
        elseif (strtolower($genre) == 'journal article') {
          $options = self::createCoinsUrlForJournalArticle($result);
        }
        else {
          $options = self::createCoinsUrlForOther($result);
        }
        $attributes = [];
        if (module_exists('googleanalytics') && !empty($result['PID'])) {
          $attributes = ['onclick' => "sendBMEventToGoogle('Search-Result-Bookmark', '" . $result['PID'] . "')"];
        }
        $options['html'] = TRUE;
        $options['attributes'] = $attributes;
        $result['coins_url'] = l($img, $coins_base_url, $options);
      }
      else {
        $result['coins_url'] = '';
      }
    }
  }

  static function createCoinsUrlForBookOrConference($result) {
    $options['query']['isbn'] = $result['solr_doc']['mods_relatedItem_host_identifier_isbn_s'];
    if (empty($options['query']['isbn'])) {
      $options['query']['isbn'] = $result['solr_doc']['mods_identifier_isbn_s'];
    }
    if (stripos((string) $result['solr_doc']['mods_genre_s'], 'chapter') !== FALSE ||
      stripos((string) $result['solr_doc']['mods_genre_s'], 'section') !== FALSE ||
      stripos((string) $result['solr_doc']['mods_genre_s'], 'conference') !== FALSE) {
      if (!empty($result['solr_doc']['mods_relatedItem_host_titleInfo_title_s'])) {
        $options['query']['title'] = $result['solr_doc']['mods_relatedItem_host_titleInfo_title_s'];
        $options['query']['genre'] = stripos((string) $result['solr_doc']['mods_genre_s'], 'conference') !== FALSE ? 'proceeding' : 'bookitem';
        if(!empty($result['solr_doc']['mods_titleInfo_title_s'])) {
          $options['query']['atitle'] = $result['solr_doc']['mods_titleInfo_title_s'];
        }
      }
    }
    else {
      if (!empty($result['solr_doc']['mods_titleInfo_title_s'])) {
        $options['query']['title'] = $result['solr_doc']['mods_titleInfo_title_s'];
        $options['query']['genre'] = 'book';
      }
    }
    self::createPagesParameter($options, $result);
    if (!empty($result['solr_doc']['mods_relatedItem_host_originInfo_dateIssued_s'])) {
      $options['query']['date'] = $result['solr_doc']['mods_relatedItem_host_originInfo_dateIssued_s'];
    }
    if (!empty($result['solr_doc']['mods_canonical_date_issued_s'])) {
      $options['query']['date'] = $result['solr_doc']['mods_canonical_date_issued_s'];
    }
    self::getAuthors($options, $result);
    self::getDoiFromResult($options, $result);
    return $options;
  }

  static function createCoinsUrlForJournalArticle($result) {
    if (!empty($result['solr_doc']['mods_relatedItem_host_identifier_issn_s'])) {
      $options['query']['issn'] = $result['solr_doc']['mods_relatedItem_host_identifier_issn_s'];
    }
    $options['query']['genre'] = 'article';
    if (!empty($result['solr_doc']['mods_identifier_issn_s'])) {
      $options['query']['issn'] = $result['solr_doc']['mods_identifier_issn_s'];
    }
    if (!empty($result['solr_doc']['mods_titleInfo_title_s'])) {
      $options['query']['atitle'] = $result['solr_doc']['mods_titleInfo_title_s'];
    }
    if (!empty($result['solr_doc']['mods_relatedItem_host_titleInfo_title_s'])) {
      $options['query']['stitle'] = $result['solr_doc']['mods_relatedItem_host_titleInfo_title_s'];
    }
    if (!empty($result['solr_doc']['mods_part_detail_volume_number_s'])) {
      $options['query']['volume'] = $result['solr_doc']['mods_part_detail_volume_number_s'];
    }
    if (!empty($result['solr_doc']['mods_part_detail_issue_number_s'])) {
      $options['query']['issue'] = $result['solr_doc']['mods_part_detail_issue_number_s'];
    }
    self::createPagesParameter($options, $result);

    if (!empty($result['solr_doc']['mods_relatedItem_host_originInfo_dateIssued_s'])) {
      $options['query']['date'] = $result['solr_doc']['mods_relatedItem_host_originInfo_dateIssued_s'];
    }
    if (!empty($result['solr_doc']['mods_canonical_date_issued_s'])) {
      $options['query']['date'] = $result['solr_doc']['mods_canonical_date_issued_s'];
    }
    self::getDoiFromResult($options, $result);
    self::getAuthors($options, $result);
    return $options;
  }

  static function createCoinsUrlForOther($result) {
    if (!empty($result['solr_doc']['mods_relatedItem_host_originInfo_dateIssued_s'])) {
      $options['query']['date'] = $result['solr_doc']['mods_relatedItem_host_originInfo_dateIssued_s'];
    }
    if (!empty($result['solr_doc']['mods_genre_s'])) {
      $options['query']['genre'] = $result['solr_doc']['mods_genre_s'];
    }
    if (!empty($result['solr_doc']['mods_relatedItem_host_titleInfo_title_s'])) {
      $options['query']['stitle'] = $result['solr_doc']['mods_relatedItem_host_titleInfo_title_s'];
    }
    if (!empty($result['solr_doc']['mods_titleInfo_title_s'])) {
      $options['query']['atitle'] = $result['solr_doc']['mods_titleInfo_title_s'];
    }
    self::getAuthors($options, $result);
    if (!empty($result['solr_doc']['mods_relatedItem_host_originInfo_dateIssued_s'])) {
      $options['query']['date'] = $result['solr_doc']['mods_relatedItem_host_originInfo_dateIssued_s'];
    }
    if (!empty($result['solr_doc']['mods_canonical_date_issued_s'])) {
      $options['query']['date'] = $result['solr_doc']['mods_canonical_date_issued_s'];
    }
    self::createPagesParameter($options, $result);
    self::getDoiFromResult($options, $result);
    return $options;
  }

  static function getDoiFromResult(&$options, $result) {
    if (!empty($result['solr_doc']['mods_identifier_doi_s'])) {
      $_doi = substr($result['solr_doc']['mods_identifier_doi_s'], 0, strlen('doi:')) === 'doi:' ? $result['solr_doc']['mods_identifier_doi_s']
        : 'doi:' . $result['solr_doc']['mods_identifier_doi_s'];
      $options['query']['ID'] = $_doi;
    }
  }

  /**
   * Get the authors from the solr search results array.
   *
   * @param array $options
   * @param array $result
   */
  static function getAuthors(&$options, $result) {
    $authors = empty($result['solr_doc']['mods_author_lastname_first_ms']) ? $result['solr_doc']['mods_author_lastname_first_mt'] : $result['solr_doc']['mods_author_lastname_first_ms'];
    if(empty($authors)) {
      return;
    }
    //solr search supplys an array, metadata module will supply an string so check
    if (is_array($authors)){
      $options['query']['authors'] = implode(';', $authors);
    } else {
      $options['query']['authors'] = self::substituteValueForComma($authors, '; ');
    }
  }

  /**
   * Replace every second comma in a string (subject) with the specified value ($value)
   *
   * @param string $subject
   * @param string $value
   *
   * @return mixed
   */
  static function substituteValueForComma($subject, $value) {
    return preg_replace('/(\w+,.*?\w),/', '$1' . $value, $subject);
  }

  static function createPagesParameter(&$options, $result) {
    if (!empty($result['solr_doc']['mods_part_extent_start_s'])) {
      $options['query']['spage'] = $result['solr_doc']['mods_part_extent_start_s'];
    }
    if (!empty($result['solr_doc']['mods_part_extent_end_s'])) {
      $options['query']['epage'] = $result['solr_doc']['mods_part_extent_end_s'];
    }
    if (!empty($options['query']['spage'])) {
      $options['query']['pages'] = $options['query']['spage'];
    }
    if (!empty($options['query']['epage'])) {
      $options['query']['pages'] .= ' - ' . $options['query']['epage'];
    }
  }

}
