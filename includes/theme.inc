<?php
// vim: set filetype=php expandtab tabstop=2 shiftwidth=2 autoindent smartindent:
/**
 * @file
 * Collection page theme modifications.
 */

/**
 * Implements theme_preprocess_page().
 */
function islandora_solr_content_type_preprocess_page(&$vars) {
	// This code allows page level theming based on the type of node.
	if (isset($vars['node']) && $vars['node']->type == 'islandora_solr_content_type' && arg(1) != 'edit') {
		$vars['theme_hook_suggestions'][] = 'page__node__' . $vars['node']->type;
	}
}

/**
 * Implements hook_theme().
 */
function islandora_solr_content_type_theme($existing, $type, $theme, $path) {
  $theme = array();
  
  // Content page template (i.e. what the end-user sees)
  $theme['page__node__islandora_solr_content_type'] = array(
    'render element' => 'content',
    'base hook' => 'page',
    'template' => 'page--node--islandora_solr_content_type',
    'path' => drupal_get_path('module', 'islandora_solr_content_type') . '/templates',
   );

  // //Node add template for admins.
  // $theme['page__node__add__islandora_solr_content_type'] = array(
  //   'render element' => 'content',
  //   'base hook' => 'page',
  //   'template' => 'page--node--add--islandora_solr_content_type',
  //   'path' => drupal_get_path('module', 'islandora_solr_content_type') . '/templates',
  //  );

  return $theme;
}


?>
