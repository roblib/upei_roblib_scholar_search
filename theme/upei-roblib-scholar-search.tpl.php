<?php
/**
 * @file
 * UPEI roblib custom results template file.
 *
 * Variables available:
 * - $results: Primary profile results array
 *
 * @see template_preprocess_islandora_solr()
 */

?>

<?php if (empty($results)): ?>
  <p class="no-results"><?php print t('Sorry, but your search returned no results.'); ?></p>
<?php else: ?>
  <div class="islandora islandora-solr-search-results">
    <?php $row_result = 0; ?>
    <?php foreach($results as $key => $result): ?>
      <!-- Search result -->
      <div class="islandora-solr-search-result clear-block <?php print $row_result % 2 == 0 ? 'odd' : 'even'; ?>">
        <div class="islandora-solr-search-result-inner">
          <!-- Thumbnail -->
          <dl class="solr-thumb">
            <dt>
              <?php print $result['thumbnail']; ?>

          <!-- begin tooltip code -->

<?php //pre/post-print tooltips 

//convert to lowercase
$pub_status = strtolower( $solr_fields['mods_physicalDescription_s']['value'][0] );
$preprint = '<div class="tooltip-item">
		<span class="pub_status"> Pre-print </span>
		<i class="fa fa-question-circle" aria-hidden="true"></i>
		<div class="tooltip">
			<div class="tooltip-content">
				"PRE-PRINT":<br> Authors Original Draft which is intended for Formal Publication, or already submitted for publication, but prior to the Accepted Work.
			</div>
		</div>
		</div>';
$postprint = '<div class="tooltip-item">
		<span class="pub_status"> Post-print </span>
		<i class="fa fa-question-circle" aria-hidden="true"></i>
		<div class="tooltip">
			<div class="tooltip-content">
			"POST-PRINT":<br> A version after peer review and acceptance. The Accepted Work or the Definitive Work or a Minor Revision.
			</div>
		</div>
		</div>';

if ( $pub_status == 'pre-publication' || $pub_status == 'pre-print' ) { print $preprint; } 
if ( $pub_status == 'post-publication' || $pub_status == 'post-print' ) { print $postprint; } 
?>
          <!-- end tooltip code -->
            </dt>
            <dd></dd>
            <dt class="solr-label roblib-coins">
              <?php print $result['coins_url']; ?>
            </dt>
            <dd></dd>
            <dt class="solr-label roblib-fulltext">
              <?php print $result['full_text_url']; ?>
            </dt>
            <dd></dd>
            <dt class="solr-label roblib-bookmark-form">
              <?php print $result['roblib_bookmark_form']; ?>
            </dt>
            <dd></dd>
          </dl>
          <!-- Metadata -->
          <dl class="solr-fields islandora-inline-metadata">
            <?php foreach($result['solr_doc'] as $key => $value): ?>
              <dt class="solr-label <?php print $value['class']; ?>">
                <?php print $value['label']; ?>
              </dt>
              <dd class="solr-value <?php print $value['class']; ?>">
                <?php print $value['value']; ?>
              </dd>
            <?php endforeach; ?>

          </dl>

        </div>
      </div>
    <?php $row_result++; ?>
    <?php endforeach; ?>
  </div>
<?php endif; ?>
