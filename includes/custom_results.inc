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
    // create the coins url before islandora_solr modifies values like title (in case title is configured to be used as a link etc.).
    $this->createCoinsUrlForResults($solr_results);
    $this->createFullTextLink($solr_results);
    $solr_results = islandora_solr_prepare_solr_results($solr_results);
    $object_results = $solr_results['response']['objects'];
    $object_results = islandora_solr_prepare_solr_doc($object_results);
    $mod_path = drupal_get_path('module', 'islandora_solr_config');
    drupal_add_css("$mod_path/css/islandora_solr_config.theme.css");

    $elements = array();
    $elements['solr_total'] = $solr_results['response']['numFound'];
    $elements['solr_start'] = $solr_results['response']['start'];

    return theme('upei_roblib_scholar_search_custom', array(
      'results' => $object_results,
      'elements' => $elements,
    ));
  }

  function createFullTextLink(&$solr_result) {
    // TODO We may want to make the base_url configurable in case we ever change resolvers.
    $results = &$solr_result['response']['objects'];
    foreach ($results as &$result) {
      if (!empty($result['solr_doc']['mods_location_url_s'])) {
        $options['html'] = TRUE;
        $options['attributes']['title'] = 'Full Text';
        // TODO we will probably want the list of datastreams so we can make a more informed decision here
        // waiting for feedback from library on whether or not we will load a citation for each search result
        // if we are then we will have to get the mods anyway so we can check for datastreams on the object
        // if we don't need the citations than i can include the datastream solr fields and use them
        // (we'll have to suppres them from the view though)
        $result['full_text_url'] = l(t("Full Text"), $result['solr_doc']['mods_location_url_s'], $options);
      }
      else {
        $result['full_text_url'] = '';
      }
    }
  }


  /**
   * Creates a url based on coins specification.
   *
   * We could use the citeproc converter to grab this data from mods but it will be expensive
   * as it will involve a round trip to Fedora for every search result.  If we need to generate
   * a citation for each search result we should use citeproc.  If not we should try to
   * get the relevant information from the solr results (for now using solr results)
   *
   * @param $pid
   */
  function createCoinsUrlForResults(&$solr_result) {
    // TODO We may want to make the base_url configurable in case we ever change resolvers.
    $coins_base_url = 'http://articles.library.upei.ca:7888/godot/hold_tab.cgi';
    $results = &$solr_result['response']['objects'];
    $img_variables = array(
      'path' => 'http://library.upei.ca/sites/all/files/sharedfolder/files/getit_upei.gif',
      'alt' => 'Get it at UPEI',
      'title' => 'Get it at UPEI',
      'attributes' => array('class' => 'coins-img',),
    );
    $img = theme('image', $img_variables);
    foreach ($results as &$result) {
      $genre = empty($result['solr_doc']['mods_genre_s']) ? NULL : $result['solr_doc']['mods_genre_s'];
      if (!empty($genre)) {
        if (stripos((string) $genre, 'book') !== FALSE && !empty($result['solr_doc']['mods_identifier_isbn_s'])) {
          $options = $this->createCoinsUrlForBook($result);
        }
        elseif ($genre == 'Journal Article') {
          $options = $this->createCoinsUrlForJournalArticle($result);
        }
        else {
          $options = $this->createCoinsUrlForOther($result);
        }
        $options['query']['hold_tab_branch'] = 'PCU';
        $options['html'] = TRUE;
        $options['attributes']['title'] = $result['object_label'];
        $result['coins_url'] = l($img, $coins_base_url, $options);
      }
      else {
        $result['coins_url'] = '';
      }
    }
  }

  function createCoinsUrlForBook($result) {
    $options['query']['isbn'] = $result['solr_doc']['mods_identifier_isbn_s'];
    if (!empty($result['solr_doc']['mods_titleInfo_title_s'])) {
      $options['query']['title'] = $result['solr_doc']['mods_titleInfo_title_s'];
    }
    if (!empty($result['solr_doc']['mods_part_extent_start_s'])) {
      $options['query']['spage'] = $result['solr_doc']['mods_part_extent_start_s'];
    }
    return $options;
  }

  function createCoinsUrlForJournalArticle($result) {
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
    if (!empty($result['solr_doc']['mods_part_extent_start_s'])) {
      $options['query']['spage'] = $result['solr_doc']['mods_part_extent_start_s'];
    }
    return $options;
  }

  function createCoinsUrlForOther($result) {
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
      $options['query']['title'] = $result['solr_doc']['mods_titleInfo_title_s'];
    }
    return $options;
  }
}
