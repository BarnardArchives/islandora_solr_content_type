<?php 
// vim: set filetype=php expandtab tabstop=2 shiftwidth=2 autoindent smartindent:
/**
 * @file
 * Collection content page display and config.
 */


function islandora_solr_content_type_prepare_content($node) {
	global $_islandora_solr_content_type;
	
	// Get our field collections.
	$display_options_collection = field_get_items('node', $node, 'isct_fc_display_opts');
	$featured_image_collection = field_get_items('node', $node, 'isct_fc_featured_image');
	$display_options = field_collection_item_load($display_options_collection[0]['value']);
	$featured_image = field_collection_item_load($featured_image_collection[0]['value']);

	// Configure the solr facets that will be displayed on the page.
	foreach (field_get_items('field_collection_item', $display_options, 'isct_display_facets') as $field) {
		$_islandora_solr_content_type['facets'][] = $field['value'];
	}

	// Define and save our starting solr query, and results per page.
	$isct_solr_query = field_get_items('node', $node, 'isct_solr_query');
	$isct_query_rows = field_get_items('node', $node, 'isct_query_rows');
	$isct_display_dateslider = field_get_items('field_collection_item', $display_options, 'isct_display_dateslider');
	$isct_display_auto_expand_date = field_get_items('field_collection_item', $display_options, 'isct_display_auto_expand_date');

	$_islandora_solr_content_type['content_query'] = $isct_solr_query[0]['value'];
	$_islandora_solr_content_type['rows'] = $isct_query_rows[0]['value'];
	$_islandora_solr_content_type['display_dateslider'] = $isct_display_dateslider[0]['value'];
	$_islandora_solr_content_type['display_expand_date'] = $isct_display_auto_expand_date[0]['value'];
}

/**
 * Implements hook_node_view().
 * This is where we display our content type to the end-user.
 */
function islandora_solr_content_type_node_view($node, $view_mode = 'full') {
	if ($node->type == 'islandora_solr_content_type') {
		dpm($node);

		// Get our globals.
		global $_islandora_solr_content_type;
		global $_islandora_solr_queryclass;

		// We're on a islandora_solr_content_type page...
		$_islandora_solr_content_type['view'] = TRUE;

		islandora_solr_content_type_prepare_content($node);		

		// For our content type, we want to default to the grid view. If the user
    // selects something else we do not want to revert back to the grid view.
		$_GET['display'] = !isset($_GET['display']) ? 'grid' : NULL;

		// The important stuff:
		// We rely on islandora_solr() to return a search result - that will be our
    // primary content. This reliance on islandora_solr let us do amazing
    // things fairly simply.
		
		// We use a param called "cq" if the user has preformed a search from our
    // collection page. If it's not there we display the content as it was
    // configured by the curator.
		if (!in_array('cq', array_keys(drupal_get_query_parameters()))) {
			$solr_result_output = islandora_solr();
		}
		else {
			$query_params = drupal_get_query_parameters();
			$collection_query = $query_params['cq'];
			$solr_result_output = islandora_solr($collection_query);
		}

		// Build the rest of our block's content - include the switch (list/grid) and sort blocks.
		$node->content['switch_block'] = array(
			'#markup' => islandora_solr_content_type_block_content_render('islandora_solr', 'display_switch'),
			'#weight' => '2',
		);

		$node->content['sort_order_block'] = array(
			'#markup' => islandora_solr_content_type_block_content_render('islandora_solr', 'sort'),
			'#weight' => '2',
		);

		$node->content['results'] = array(
			'#markup' => $solr_result_output,
			'#weight' => '3',
		);

	}
}

/*
 * Implements hook_block_list_alter().
 */
function islandora_solr_content_type_block_list_alter(&$blocks) {
	global $_islandora_solr_content_type;
	global $theme_key;

	// If it is not one of our unique queries, return;
	if (!$_islandora_solr_content_type['view']) {
		return;
	}

	// Find the display_switch block that, for some unknown reason, is 
	// displayed on all pages on our site (or by default)
	// We do not want *this* display_switch block, since 
	// we already render and add it to our pages ['content'].
	foreach($blocks as $block) {
		if($block->delta == 'display_switch') {
			unset($blocks[$block->bid]);
		}
	}

	// We're going to add our sidebar content, mainly the facet block and
  // advanced search block. Both of these are from the islandora_solr module.

	$additional_blocks = array(
    'islandora_solr' => array(
			'advanced' => array(
        'title' => 'Search this collection',
        'region' => 'sidebar_first',
        'weight' => 0,
      ),
      'basic_facets' => array(
        'title' => 'Filter this collection',
        'region' => 'sidebar_first',
        'weight' => 1,
      ),
		),
	);

	// Fetch the additional blocks, setting the theme, region and weight.	
	foreach ($additional_blocks as $module_name => $addl_block) {
		foreach ($addl_block as $bid => $block_info) {
      $block = block_load($module_name, $bid);
      $block->theme = $theme_key;
      $block->title = $block_info['title'];
      $block->region = $block_info['region'];
      $block->weight = $block_info['weight'];
      $blocks[] = $block;
		}
	}

  // Sort the blocks
  usort($blocks, "islandora_solr_content_type_custom_sort");
}

/**
 * Sorting function for blocks as objects. Why drupal only has sort functions for arrays is beyond me...
 */
function islandora_solr_content_type_custom_sort($a,$b) {
  return $a->weight>$b->weight;
}

/**
 * Generates a rendered block from $module with name $function.
 */
function islandora_solr_content_type_block_content_render($module, $function) {
	$block = block_load($module, $function);
	$block_renderable = _block_get_renderable_array(_block_render_blocks(array($block)));
	$render = drupal_render($block_renderable);
	return $render;	
}

/**
 * Implements hook_form_BASE_ID_alter().
 * For our content type we want to simplify the facet block by hiding a few elements.
 */
function islandora_solr_content_type_form_islandora_solr_range_slider_form_alter(&$form, &$form_state) {
	global $_islandora_solr_content_type;

	// If it is not one of our unique queries, return;
	if (!$_islandora_solr_content_type['view']) {
		return;
	}

	// We need to access the theme layer to hide the "Date Created" display.
	if (!$_islandora_solr_content_type['display_dateslider']) {
		// Get rid of datepicker bar and the corresponding submit.
	 	$form['markup']['#access'] = FALSE;
		$form['range_slider_submit']['#access'] = FALSE;
	}

}

/**
 * Implements hook_form_BASE_ID_alter()
 * For our content type we want to simplify the facet block by hiding a few elements.
 */
function islandora_solr_content_type_form_islandora_solr_date_filter_form_alter(&$form, &$form_state) {
	global $_islandora_solr_content_type;

	// If it is not one of our unique queries, return;
	if (!$_islandora_solr_content_type['view']) {
		return;
	}

	if ($_islandora_solr_content_type['display_expand_date']) {
		// Modify the markup to class 'expanded' and remove the hide/show link:
		$form['date_range_expand']['#markup'] = '<span class="toggle-date-range-filter date-range-expanded"></span>';
	}	
}

/**
 * Implements hook_islandora_solr_query().
 */
function islandora_solr_content_type_islandora_solr_query($islandora_solr_query) {
	global $_islandora_solr_content_type;
	if (!$_islandora_solr_content_type['view']) {
		return;
	}

	// Has the user entered a search term? Here is where we modify our collection
  // view... First we need to find if any strings were used in the solr fq, and
  // change them to tokenized fields (i.e. 'ms' becomes 'mt'). Then we set our
  // solr query to the user's term with a catch-all so we get as much for our
  // search as possible. Finally, we return the ISQP and stop the function from
  // continuing.
	if (in_array('cq', array_keys(drupal_get_query_parameters()))) {

		// Tokenize and set field query limit.
		$content_query = str_replace('_ms:', '_mt:', $_islandora_solr_content_type['content_query']);
		$islandora_solr_query->solrParams['fq'][] = $content_query;

		// Return our search ISQP.
		return $islandora_solr_query;
	} 

	// Set field query limit.
	$islandora_solr_query->solrParams['fq'][] = $_islandora_solr_content_type['content_query'];

	return $islandora_solr_query;
}

/**
 * Implements hook_islandora_solr_query_alter().
 * Modifies query before execution.
 */
function islandora_solr_content_type_islandora_solr_query_alter(&$islandora_solr_query) {
	global $_islandora_solr_content_type;

	// If it is not one of our unique queries, return.
	if (!$_islandora_solr_content_type['view']) {
		return;
	}

	// I need to figure out pagination and why 'rows' isn't restricting the
  // results from solr. This leads to pagination, not result limits.
	$islandora_solr_query->solrLimit = $_islandora_solr_content_type['rows'];

	// Update facets to only those selected...
	$islandora_solr_query->solrParams['facet.field'] = $_islandora_solr_content_type['facets'];
}

/**
 * Implements hook_islandora_solr_query_result().
 */
// function islandora_solr_content_type_islandora_solr_query_result($result) {
// }

/**
 * Implements hook_islandora_solr_results_alter().
 */
// function islandora_solr_content_type_islandora_solr_results_alter(&$search_results, $query_processor) {
// }

?>
