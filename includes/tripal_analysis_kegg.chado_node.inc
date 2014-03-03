<?php 

/*******************************************************************************
 *  Provide information to drupal about the node types that we're creating
*  in this module
*/
function tripal_analysis_kegg_node_info() {
  $nodes = array();
  $nodes['chado_analysis_kegg'] = array(
      'name' => t('Analysis: KEGG'),
      'base' => 'chado_analysis_kegg',
      'description' => t('Results from a KEGG/KAAS analysis'),
      'has_title' => FALSE,
      'title_label' => t('Analysis: KEGG'),
      'has_body' => FALSE,
      'body_label' => t('KEGG Analysis Description'),
      'locked' => TRUE
  );
  return $nodes;
}

/*******************************************************************************
 *  The following function proves access control for users trying to
*  perform actions on data managed by this module
*/
function chado_analysis_kegg_access($op, $node, $account) {
  if ($op == 'create') {
    if (!user_access('create chado_analysis_kegg content', $account)) {
      return FALSE;
    }
  }
  if ($op == 'update') {
    if (!user_access('edit chado_analysis_kegg content', $account)) {
      return FALSE;
    }
  }
  if ($op == 'delete') {
    if (!user_access('delete chado_analysis_kegg content', $account)) {
      return FALSE;
    }
  }
  if ($op == 'view') {
    if (!user_access('access chado_analysis_kegg content', $account)) {
      return FALSE;
    }
  }
  return NULL;
}

/*******************************************************************************
 *  Provide a KEGG Analysis form
*/
function chado_analysis_kegg_form($node, $form_state) {

  // add in the default fields
  $form = chado_analysis_form($node, $form_state);
  
  // set the defaults
  $hierfile = '';
  $query_re = '';
  $query_type = '';
  $query_uniquename = '';
  
  if (property_exists ( $node, 'analysis' )) {
    $kegg = $node->analysis->tripal_analysis_kegg;
    $query_re = $kegg->query_re;
    $query_type = $kegg->query_type;
    $query_uniquename = $kegg->query_uniquename;    
    $hierfile = $kegg->hierfile;
  }

  $moreSettings ['kegg'] = 'KEGG Analysis Settings';
  $form['kegg'] = array(
      '#title' => t('KEGG Settings'),
      '#type' => 'fieldset',
      '#description' => t('Specific Settings for KEGG Analysis.'),
      '#collapsible' => TRUE,
      '#attributes' => array('id' => 'kegg-extra-settings'),
      '#weight' => 11
  );
  $form['kegg']['hierfile'] = array(
      '#title' => t('KAAS hier.tar.gz Output File'),
      '#type' => 'textfield',
      '#description' => t('The full path to the hier.tar.gz file generated by KAAS.
          Alternatively, you can input the full path to the directory
          that contains decompressed kegg files.'),
      '#default_value' => $hierfile,
  );
  $form['kegg']['query_re'] = array(
      '#title' => t('Query Name RE'),
      '#type' => 'textfield',
      '#description' => t('Enter the regular expression that will extract the '.
          'feature name from the results line in the KEGG heir results. This will be '.
          'the same as the definition line in the query FASTA file used for the analysis.  This option is '.
          'is only required when the query does not identically match a feature '.
          'in the database.'),
      '#default_value' => $query_re,
  );

  $form['kegg']['query_type'] = array(
      '#title' => t('Query Type'),
      '#type' => 'textfield',
      '#description' => t('Please enter the Sequence Ontology term that describes '.
          'the query sequences used for KEGG.  This is only necessary if two '.
          'or more sequences have the same name.'),
      '#default_value' => $query_type,
  );

  $form['kegg']['query_uniquename'] = array(
      '#title' => t('Use Unique Name'),
      '#type' => 'checkbox',
      '#description' => t('Select this checboxk if the feature name in the KEGG heir file '.
          'matches the uniquename in the database.  By default, the feature will '.
          'be mapped to the "name" of the feature.'),
      '#default_value' => $query_uniquename,
  );
  $form['kegg']['keggjob'] = array(
      '#type' => 'checkbox',
      '#title' => t('Submit a job to parse the kegg output into Chado'),
      '#description' => t('Note: features used in the KAAS analysis must '.
          'exist in chado before parsing the file. Otherwise, KEGG '.
          'results that cannot be linked to a feature will be '.
          'discarded.'),
  );
  return $form;
}
/**
 *
 */
function chado_analysis_kegg_validate($node, &$form, &$form_state) {
  // use the analysis parent to validate the node
  tripal_analysis_validate($node, $form, $form_state);
}
/*******************************************************************************
 *
*/
function chado_analysis_kegg_insert($node) {

  // get the analysis_id from the node before the call below
  if (property_exists($node, 'analysis_id')) {
    $analysis_id = $node->analysis_id;
  }

  // insert the analysis. If the analysis already exist then this
  // call will link it to a new Drupa node.
  chado_analysis_insert($node);

  // If this analysis already exists then don't recreate it in chado. This can happen
  // when nodes are synced.  They already exist but Drupal doesn't know it and calls
  // this function.
  if (!$analysis_id) {

  // now add in the remaining settings as a single property but separated by bars
  tripal_analysis_insert_property($node->analysis_id, 'analysis_kegg_settings', trim($node->hierfile));
  tripal_analysis_insert_property($node->analysis_id, 'analysis_kegg_query_re', trim($node->query_re));
  tripal_analysis_insert_property($node->analysis_id, 'analysis_kegg_query_type', trim($node->query_type));
  tripal_analysis_insert_property($node->analysis_id, 'analysis_kegg_query_uniquename', trim($node->query_uniquename));
   
  // Add a job if the user wants to parse the html output
  chado_analysis_kegg_submit_job($node);
  }
}
  /**
  *
  */
function chado_analysis_kegg_submit_job($node) {
  global $user;
  global $base_url;

    if ($node->keggjob) {
    $job_args[0] = $node->analysis_id;
      $job_args[1] = $node->hierfile;
      $job_args[2] = $base_url;
      $job_args[3] = $node->query_re;
      $job_args[4] = $node->query_type;
      $job_args[5] = $node->query_uniquename;

      if (is_readable($node->hierfile)) {
      $fname = preg_replace("/.*\/(.*)/", "$1", $node->hierfile);
      tripal_add_job("Parse KAAS output: $fname", 'tripal_analysis_kegg',
      'tripal_analysis_kegg_parseHierFile', $job_args, $user->uid);
    }
    else {
      drupal_set_message("Can not open KAAS hier.tar.gz output file. Job not scheduled.");
    }
  }
}
/*******************************************************************************
* Delete KEGG anlysis
*/
function chado_analysis_kegg_delete($node) {
  chado_analysis_delete($node);
}

/*******************************************************************************
      * Update KEGG analysis
      */
function chado_analysis_kegg_update($node) {
  // insert the analysis
  chado_analysis_update($node);

  // now add in the remaining settings as a single property but separated by bars
  tripal_analysis_update_property($node->analysis_id, 'analysis_kegg_settings', trim($node->hierfile), 1);
  tripal_analysis_update_property($node->analysis_id, 'analysis_kegg_query_re', trim($node->query_re), 1);
  tripal_analysis_update_property($node->analysis_id, 'analysis_kegg_query_type', trim($node->query_type), 1);
  tripal_analysis_update_property($node->analysis_id, 'analysis_kegg_query_uniquename', trim($node->query_uniquename), 1);

  // Add a job if the user wants to parse the output
  chado_analysis_kegg_submit_job($node);
}
/*******************************************************************************
*  When a node is requested by the user this function is called to allow us
*  to add auxiliary data to the node object.
*/
function chado_analysis_kegg_load($nodes) {
    
  // load the default set of analysis fields
  chado_analysis_load ( $nodes );
  
  foreach ( $nodes as $nid => $node ) {
    // create some variables for easier lookup
    $analysis = $node->analysis;
    $analysis_id = $analysis->analysis_id;
    
    // get the heirfile name
    $hierfile = tripal_analysis_get_property ( $analysis_id, 'analysis_kegg_settings' );
    $query_re = tripal_analysis_get_property ( $analysis->analysis_id, 'analysis_kegg_query_re' );
    $query_type = tripal_analysis_get_property ( $analysis->analysis_id, 'analysis_kegg_query_type' );
    $query_uniquename = tripal_analysis_get_property ( $analysis->analysis_id, 'analysis_kegg_query_uniquename' );
    
    $analysis->tripal_analysis_kegg = new stdClass ();    
    $analysis->tripal_analysis_kegg->hierfile = is_object($hierfile)   ? $hierfile->value : '';
    $analysis->tripal_analysis_kegg->query_re = is_object($query_re)  ? $query_re->value: '';
    $analysis->tripal_analysis_kegg->query_type = is_object($query_type)  ? $query_type->value : '';
    $analysis->tripal_analysis_kegg->query_uniquename = is_object($query_uniquename)  ? $query_uniquename->value : '';
    
    $nodes[$nid]->analysis = $analysis;
  }
}

      /**
       *
       */
function chado_analysis_kegg_view($node, $teaser = FALSE, $page = FALSE) {
  // use drupal's default node view:
  if (!$teaser) {
    $node = node_prepare($node, $teaser);
    // When previewing a node submitting form, it shows 'Array' instead of
    // correct date format. We need to format the date here
    $time = $node->timeexecuted;
    if (is_array($time)) {
      $month = $time['month'];
      $day = $time['day'];
      $year = $time['year'];
      $timestamp = $year . '-' . $month . '-' . $day;
      $node->timeexecuted = $timestamp;
    }
  }
  return $node;
}


/*******************************************************************************
 * HOOK: Implementation of hook_nodeapi()
* Display library information for associated features or organisms
* This function also provides contents for indexing
*/
function tripal_analysis_kegg_node_view($node, $view_mode, $langcode) {
  switch ($node->type) {
    case 'chado_analysis_kegg':
      if ($view_mode == 'full') {
        $node->content['tripal_analysis_kegg_base'] = array(
            '#markup' => theme('tripal_analysis_kegg_base', array('node' => $node)),
            '#tripal_toc_id'    => 'base',
            '#tripal_toc_title' => 'Overview',
            '#weight' => -100,
        );
        $node->content['tripal_analysis_kegg_report'] = array(
            '#markup' => theme('tripal_analysis_kegg_report', array('node' => $node)),
            '#tripal_toc_id'    => 'report',
            '#tripal_toc_title' => 'KEGG Report',
        );
      }
      if ($view_mode == 'teaser') {
        $node->content['tripal_analysis_kegg_teaser'] = array(
            '#markup' => theme('tripal_analysis_kegg_teaser', array('node' => $node)),
        );
      }
    break;
    case 'chado_organism':
      // Show KEGG report on the organism page
      if ($view_mode == 'full') {
        $node->content ['tripal_organism_kegg_summary'] = array (
            '#markup' => theme('tripal_organism_kegg_summary', array('node' => $node)),
            '#tripal_toc_id'    => 'kegg_summary',
            '#tripal_toc_title' => 'KEGG Report' 
        );
      }
      break;
  }
}