<?php

/**
 * Renders the inside of the element
 * DO NOT call drupal_render($variables['element']) (i.e. on the entire element), as this will cause recursion.
 */
function theme_scripture_verse_picker($variables) {
  $output = "";
  $e = &$variables['element'];

//   $output .= drupal_render($e['preview']);
  $t = drupal_render($e['translation']);
  $b = drupal_render($e['book']);
  $fc = drupal_render($e['from_chapter']);
  $fv = drupal_render($e['from_verse']);
  $tc = drupal_render($e['to_chapter']);
  $tv = drupal_render($e['to_verse']);
  $s = drupal_render($e['sid']);
  
  $output .= "<div class='scripture-translation'>{$t}</div><div class='scripture-book-verse'>{$b} {$fc} : ${fv}";
  
  if (strlen($tv)) {
    if (strlen($tc)) {
      $output .= " - {$tc} : {$tv}";
    } else {
      $output .= " - {$tv}";
    }
  }
  
  if (strlen($s)) {
    $output .= " ({$s})";
  }
  
  $output .= "</div>";
  
  return $output;
}

/**
 * Render inline wrappers form elements
 */
function theme_scripture_inline_wrapper($var) {
  $tooltip = $var['element']['#title'];
  if (!empty($var['element']['#description'])) {
    $tooltip .= ": " . $var['element']['#description'];
  }
  return "<span title='{$tooltip}'>{$var['element']['#children']}</span>";
}

/**
 * gets called when verse picker element is used in a form
 */
function scripture_verse_picker_process($element, &$form_state, $complete_form) {
  
  // Element settings
  $settings = empty($element['#scripture_settings']) ? array() : $element['#scripture_settings'];
  $settings += array(
    'require_translation' => FALSE, // translation is optional if FALSE
    'allow_translation'   => TRUE,  // translation cannot be edited if FALSE
    'allow_book'          => TRUE,  // book cannot be edited if FALSE
    'allow_range'         => TRUE,  // only a single verse may be selected if FALSE
    'allow_subverse'      => TRUE,  // sid field is hidden if FALSE
  );

  // Default values to populate element
  $init = empty($element['#value']) ? array() : $element['#value'];
  $init += empty($element['#default_value']) ? array() : $element['#default_value'];
  $init += array(
    'translation' => NULL,
    'book' => NULL,
    'from_chapter' => '',
    'from_verse' => '',
    'to_chapter' => '',
    'to_verse' => '',
    'sid' => '',
  );

  // Options for select lists
  $translations = scripture_get_translations();
  foreach ($translations as $id => &$t) {
    $t = "{$t->abbr} / {$t->lang} / {$t->name}";
  }
  if (!isset($translations[$init['translation']])) {
    $init['translation'] = NULL;
  }
  $books = scripture_get_books($init['translation']);
  foreach ($books as $booknum => &$b) {
    $b = "{$booknum}. {$b->bookname}";
  }
  if (!isset($books[$init['book']])) {
    $init['book'] = NULL;
  }

  // TODO: http://drupal.stackexchange.com/questions/162582/making-a-custom-form-element-ajax-aware

  // The compound form element:
  
  $element['#tree'] = TRUE;
  
  $book_wrapper = drupal_html_id('scripture_verse_picker_book');

  $element['translation'] = array(
    '#type' => 'select',
    '#title' => t('Translation:'),
    '#title_display' => 'before',
    '#ajax' => array(
      'wrapper'  => $book_wrapper,
      'callback' => 'scripture_ajax_translation_changed',
      'method'   => 'replace',
    ),
    '#disabled' => !$settings['allow_translation'],
  );
  
  if ($settings['require_translation']) {
    $element['translation']['#required'] = TRUE;
    $element['translation']['#options'] = $translations;
    $element['translation']['#default_value'] = empty($init['translation']) ? variable_get('scripture_default_translation', NULL) : $init['translation'];
  } else {
    $element['translation']['#options'] = array(0 => 'N/A') + $translations;
    $element['translation']['#default_value'] = empty($init['translation']) ? 'N/A' : $init['translation'];
  }
  
  $element['book'] = array(
    '#prefix'        => "<div id='{$book_wrapper}'>",
    '#suffix'        => "</div>",
    '#type'          => 'select',
    '#title'         => t('Book:'),
    '#options'       => $books,
    '#required'      => true,
    '#default_value' => $init['book'],
    '#disabled'      => !$settings['allow_book'],
  );
  
  $element['from_chapter'] = array(
    '#type'           => 'textfield',
    '#size'           => 3,
    '#maxlength'      => 3,
    '#required'       => true,
    '#default_value'  => $init['from_chapter'],
    '#title_display'  => 'invisible',
    '#theme_wrappers' => array('scripture_inline_wrapper'),
  );
  
  $element['from_verse'] = array(
    '#type'           => 'textfield',
    '#size'           => 3,
    '#maxlength'      => 3,
    '#required'       => true,
    '#default_value'  => $init['from_verse'],
    '#title_display'  => 'invisible',
    '#theme_wrappers' => array('scripture_inline_wrapper'),
  );
  
  $element['to_chapter'] = array(
    '#type'           => 'textfield',
    '#title'          => t('To chapter'),
    '#description'    => t('Last chapter in the range. Leave blank to use the "from chapter" value.'),
    '#size'           => 3,
    '#maxlength'      => 3,
    '#required'       => false,
    '#default_value'  => $init['to_chapter'],
    '#access'         => $settings['allow_range'],
    '#title_display'  => 'invisible',
    '#theme_wrappers' => array('scripture_inline_wrapper'),
  );
  
  $element['to_verse'] = array(
    '#type'           => 'textfield',
    '#title'          => t('To verse'),
    '#description'    => t('Last verse in the range. Leave this and "To chapter" blank to use the "from verse" value.'),
    '#size'           => 3,
    '#maxlength'      => 3,
    '#required'       => false,
    '#default_value'  => $init['to_verse'],
    '#access'         => $settings['allow_range'],
    '#title_display'  => 'invisible',
    '#theme_wrappers' => array('scripture_inline_wrapper'),
  );
    
  // TODO: hide this field (using AJAX?) if a range of verses is specified
  $element['sid'] = array(
    '#type'           => 'textfield',
    '#title'          => t('Subverse'),
    '#size'           => 1,
    '#maxlength'      => 3,
    '#required'       => false,
    '#default_value'  => $init['sid'],
    '#access'         => $settings['allow_subverse'],
    '#title_display'  => 'invisible',
    '#theme_wrappers' => array('scripture_inline_wrapper'),
  );
  
  if ($settings['allow_range']) {
    $element['from_chapter']['#title'] = t('From chapter');
    $element['from_chapter']['#description'] = t('First chapter in the range.');
    $element['from_verse']['#title'] = t('From verse');
    $element['from_verse']['#description'] = t('First verse in the range.');
    $element['sid']['#description'] = t('This will be ignored if the first and last verses and chapters are not the same.');
  } else {
    $element['from_chapter']['#title'] = t('Chapter');
    $element['from_verse']['#title'] = t('Verse');
  }

  return $element;

}

/**
 * Modify user input
 * http://drupal.stackexchange.com/questions/125386/how-does-value-callback-really-work-nested-elements
 *
 * value_callback provides a way to process values before they are returned to the form submit function.
 * You need to provide a #value_callback if you want to do fancy processing on the input.
 * If you just want #value set to #default_value, you don't need this hook at all, since that will happen anyway.
 * This function is mainly useful if you need to do something non-standard with the value, like transpose an array into an option list.
 *
 * Value callbacks should consider three conditions:
 * - Input is being provided directly. This happens when a form is submitted.
 * - No input is provided, but the field definition has a default value.
 * - No input is provided and there is no default value.
 */
function form_type_scripture_verse_picker_value($element, $input = FALSE, &$form_state) {
  
  if ($input === FALSE) {
    
    $input = isset($element['#default_value']) ? $element['#default_value'] : array();
    
  } else {
    
    // check whether a specific translation was selected
    if (empty($input['translation']) || $input['translation'] == 'N/A') {
      $input['translation'] = NULL;
    }
    
    if (empty($input['to_chapter'])) {
  
      // use the from_chapter value if to_chapter was empty
      $input['to_chapter'] = $input['from_chapter'];
  
      if (empty($input['to_verse'])) {
        // use the from_verse value if both to_chapter and to_verse were empty
        $input['to_verse'] = $input['from_verse'];
      }
  
    }
    
  }
  
  return $input;

}

/**
 * Validate user input
 * Checks the verse ranges logically
 * Checks if verses exist in database
 * Loads vids from database
 */
function scripture_verse_picker_validate($element, &$form_state) {
  
  $v = &$element['#value'];
  $errors = FALSE;
  
  // check that verse accompanies chapter
  if (!empty($v['to_chapter']) && empty($v['to_verse'])) {
    form_error($element, t('You filled in a chapter, but not a verse.'));
    $errors = TRUE;
  }
  
  if (empty($v['to_chapter'])) {
  
    // use the from_chapter value if to_chapter was empty
    $v['to_chapter'] = $v['from_chapter'];
  
    if (empty($v['to_verse'])) {
      // use the from_verse value if both to_chapter and to_verse were empty
      $v['to_verse'] = $v['from_verse'];
    }
  
  }
  
  // check that the verse range is positive
  if ($v['from_chapter'] > $v['to_chapter'] || ($v['from_chapter'] == $v['to_chapter'] && $v['from_verse'] > $v['to_verse'])) {
    form_error($element, t('First verse and chapter must be before the last verse and chapter.'));
    $errors = TRUE;
  }
  
  // if there are no other errors, check if the verses exist by requesting their vids
  // need to check for non-empty parameters, because the "required" validation criteria is not checked on individual elements when this is called with AJAX
  if (!$errors && !empty($v['from_chapter']) && !empty($v['to_chapter']) && !empty($v['from_verse']) && !empty($v['to_verse'])) {
  
    // get the verse ids using the above information
    $v['from_vid'] = scripture_get_vid($v['translation'], $v['book'], $v['from_chapter'], $v['from_verse']);
    $v['to_vid'] = scripture_get_vid($v['translation'], $v['book'], $v['to_chapter'], $v['to_verse']);
    
    // check if the verses exist, i.e. have vids
    if (!($v['from_vid'] && $v['to_vid'])) {
      // at least one verse does not exist in the range
      form_error($element, t('Requested verse does not exist.'));
      $errors = TRUE;
    }
    
    // check whether the range is a single verse
    if (empty($v['sid']) || $v['to_vid'] != $v['from_vid']) {
      $v['sid'] = NULL;
    }
    
  }
    
  if (!$errors && !empty($v['sid'])) {
    if (!scripture_subverse_exists($v['from_vid'], $v['sid'], $v['translation'])) {
      // The subverse does not exist
      form_error($element, t('Requested subverse does not exist.'));
      $errors = TRUE;
    }
  }
  
  // update the form state with the new values
  form_set_value($element, $v, $form_state);
  
  return $element;
}

/**
 * React to user changing the translation
 * Update list of books
 *
 * Check this out: https://www.drupal.org/node/279246#comment-2661792 ...
 */
function scripture_ajax_translation_changed($form, $form_state) {
  $array_parents = $form_state['triggering_element']['#array_parents'];
  array_pop($array_parents); // go one level up from the triggering element
  $scripture_verse_picker_element = drupal_array_get_nested_value($form, $array_parents);
  return $scripture_verse_picker_element['book'];
}