<?php
/**
 * @file
 * Collection content page display and config.
 */

/**
 * Parses node variables into a global variable.
 *
 * This is used throughout the display and search includes.
 */
function islandora_solr_content_type_prepare_content($node) {
  global $_islandora_solr_content_type;

  // These are our content variables. These DEFINE what is present on the
  // content page, and also help us with search. Everything gets saved in the
  // content's global - I'm sure there is a better way.

  $isct_values = array(
    'isct_solr_query', 'isct_solr_search_query', 'isct_query_rows',
    'isct_display_search_title', 'isct_display_facet_title',
    'isct_display_datefacets', 'isct_display_dateslider',
    'isct_display_auto_expand_date', 'isct_display_facets',
    'isct_featured_topic_hide',
    // 'isct_featured_content_area', // We don't need this in our global.
  );

  foreach ($isct_values as $field) {
    $field_value = field_get_items('node', $node, $field);
    $global_field_name = str_replace('isct_', '', $field);
    if ($global_field_name !== 'display_facets') {
      $_islandora_solr_content_type[$global_field_name]
        = $field_value[0]['value'];
    }
    elseif (!empty($field_value)) {
      foreach ($field_value as $facet) {
        $_islandora_solr_content_type[$global_field_name][] = $facet['value'];
      }
    }
  }

}

/**
 * Implements hook_node_view().
 *
 * This is where we display our content type to the end-user.
 */
function islandora_solr_content_type_node_view($node, $view_mode = 'full') {
  // Party on Garth. Party on.
  if ($node->type !== 'islandora_solr_content_type') {
    return;
  }

  // Get our global.
  global $_islandora_solr_content_type;

  // We're on a islandora_solr_content_type page.
  $_islandora_solr_content_type['view'] = TRUE;

  islandora_solr_content_type_prepare_content($node);
  // For our content type, we want to default to the grid view. If the user
  // selects something else do not want to override them back to the grid view.
  !isset($_GET['display']) ? $_GET['display'] = 'grid' : NULL;

  // The _very_ important stuff:
  // Foremost we rely on islandora_solr() to return a search result -
  // that will be our primary content.
  // This reliance on islandora_solr let us do amazing things fairly simply.

  // We use a param called "cq" if the user has preformed a search from our
  // collection page. If it's not there we display the content as it was
  // configured by the curator.
  if (empty($_GET['cq']) && empty($_GET['f'])) {
    $_islandora_solr_content_type['user_search'] = FALSE;
    // See hook_islandora_solr_query().
    $solr_result_output = islandora_solr();
  }
  else {
    $_islandora_solr_content_type['user_search'] = TRUE;
    // See hook_islandora_solr_query().
    $solr_result_output = islandora_solr($_GET['cq']);
  }

  $node->content['islandora_display_switch'] = array(
    '#markup' => islandora_solr_content_type_block_content_render('islandora_solr',
      'display_switch'), '#weight' => '2',
  );

   // Build the rest of our block's content - include the sort block.
   $node->content['islandora_display_sort'] = array(
     '#markup' => islandora_solr_content_type_block_content_render('islandora_solr',
       'sort'), '#weight' => '2',
   );

//  $node->content['islandora_display_sort'] = array(
//    '#markup' => islandora_solr_content_type_block_content_render('bc_islandora',
//      'solr_sort_select'), '#weight' => '3',
//  );

  $node->content['results']
    = array('#markup' => $solr_result_output, '#weight' => '4');

}

/**
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
  foreach ($blocks as $block) {
    if ($block->delta == 'display_switch') {
      unset($blocks[$block->bid]);
    }
  }

  // We're going to add our sidebar content, mainly the facet block and
  // advanced search block. Both of these are from the islandora_solr module.

  $additional_blocks = array(
    'islandora_solr' => array(
      'advanced' => array(
        'title' => $_islandora_solr_content_type['display_search_title'],
        'region' => 'sidebar_first', 'weight' => 0,
      ), 'basic_facets' => array(
        'title' => $_islandora_solr_content_type['display_facet_title'],
        'region' => 'sidebar_first', 'weight' => 1,
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

  // Sort the blocks.
  usort($blocks, "islandora_solr_content_type_custom_sort");
}

/**
 * Sorting function for blocks as objects.
 *
 * @param object $a
 *   block a.
 * @param object $b
 *   block b.
 *
 * @return bool
 *   weight of blocks.
 */
function islandora_solr_content_type_custom_sort($a, $b) {
  return $a->weight > $b->weight;
}

/**
 * Generates a rendered block from $module with name $function.
 *
 * @param string $module
 *   module name.
 * @param string $function
 *   function name.
 *
 * @return string (drupal_render)
 *  Returns the rendered drupal block from module with function.
 */
function islandora_solr_content_type_block_content_render($module, $function) {
  $block = block_load($module, $function);
  $block_renderable
    = _block_get_renderable_array(_block_render_blocks(array($block)));
  $render = drupal_render($block_renderable);

  return $render;
}

/**
 * Implements hook_form_BASE_ID_alter().
 *
 * For our content type we want to simplify the facet block by hiding a few
 * elements.
 */
function islandora_solr_content_type_form_islandora_solr_range_slider_form_alter(&$form,
                                                                                 &$form_state) {
  global $_islandora_solr_content_type;

  // If it is not one of our unique queries, return;
  if (!$_islandora_solr_content_type['view']) {
    return;
  }

  // Hide the entire date faceting area!
  // If this is TRUE once we set access we can leave this function.
  if ($_islandora_solr_content_type['display_datefacets']) {
    $form['#access'] = FALSE;

    return;
  }

  // We need to access the theme layer to hide the display of the title "Date
  // Created"
  // @see: https://github.com/Islandora/islandora_solr_search/blob/7.x/theme/theme.inc#L108
  if ($_islandora_solr_content_type['display_dateslider']) {
    // Get rid of datepicker bar and the corresponding submit.
    $form['markup']['#access'] = FALSE;
    $form['range_slider_submit']['#access'] = FALSE;
  }

}

/**
 * Implements hook_form_BASE_ID_alter().
 *
 * For our content type we want to simplify the facet block by hiding a few
 * elements.
 */
function islandora_solr_content_type_form_islandora_solr_date_filter_form_alter(&$form,
                                                                                &$form_state) {
  global $_islandora_solr_content_type;

  // If it is not one of our unique queries, return;
  if (!$_islandora_solr_content_type['view']) {
    return;
  }

  if ($_islandora_solr_content_type['display_auto_expand_date']) {
    // Modify the markup to class 'expanded' and remove the hide/show link:
    $form['date_range_expand']['#markup']
      = '<span class="toggle-date-range-filter date-range-expanded"> </span>';
  }
}

// THIS area is a KEY to our content display, and not searching within
// the collection. THEREFORE it is in content_view.inc and not search.inc.

/**
 * Implements hook_islandora_solr_query().
 */
function islandora_solr_content_type_islandora_solr_query($islandora_solr_query) {
  global $_islandora_solr_content_type;

  if (!$_islandora_solr_content_type['view']) {
    return FALSE;
  }

  // If the solr_search_query field is filled in, use that query.
  if ($_islandora_solr_content_type['user_search']) {
    $islandora_solr_query->solrParams['fq'][]
      = $_islandora_solr_content_type['solr_search_query'];
  } else {
    // Otherwise use the collection default quesry.
    $islandora_solr_query->solrParams['fq'][]
      = $_islandora_solr_content_type['solr_query'];
  }

  return $islandora_solr_query;
}

/**
 * Implements hook_islandora_solr_query_alter().
 */
function islandora_solr_content_type_islandora_solr_query_alter(&$islandora_solr_query) {
  global $_islandora_solr_content_type;

  // If it is not one of our unique queries, return.
  if (!$_islandora_solr_content_type['view']) {
    return FALSE;
  }

  // I need to figure out pagination and why 'query_rows' isn't restricting the
  // results from solr. This leads to pagination, not result limits.
  $islandora_solr_query->solrLimit
    = $_islandora_solr_content_type['query_rows'];

  // Update facets to only those selected...
  if (!empty($_islandora_solr_content_type['display_facets'])) {
    $islandora_solr_query->solrParams['facet.field']
      = $_islandora_solr_content_type['display_facets'];
  }

}

// vim: set filetype=php:
