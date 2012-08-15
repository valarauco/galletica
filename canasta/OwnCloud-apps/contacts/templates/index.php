<div id='notification'></div>
<script type='text/javascript'>
	var totalurl = '<?php echo OCP\Util::linkToRemote('carddav'); ?>addressbooks';
	var categories = <?php echo json_encode($_['categories']); ?>;
	var id = '<?php echo $_['id']; ?>';
	var lang = '<?php echo OCP\Config::getUserValue(OCP\USER::getUser(), 'core', 'lang', 'en'); ?>';
</script>
<div id="leftcontent">
	<div class="hidden" id="statusbar"></div>
	<div id="contacts">
	</div>
	<div id="uploadprogressbar"></div>
	<div id="bottomcontrols">
			<button class="svg" id="contacts_newcontact" title="<?php echo $l->t('Add Contact'); ?>"><img class="svg" src="<?php echo OCP\Util::imagePath('contacts', 'contact-new.svg'); ?>" alt="<?php echo $l->t('Add Contact'); ?>" /></button>
			<button class="svg import tip" title="<?php echo $l->t('Import'); ?>">
				<img class="svg" src="<?php echo OCP\Util::imagePath('core', 'actions/upload.svg') ?>" alt="<?php echo $l->t('Import'); ?>" />
			</button>
			<button class="svg settings tip" title="<?php echo $l->t('Settings'); ?>"><img class="svg" src="<?php echo OCP\Util::imagePath('core', 'actions/settings.svg') ?>" alt="<?php echo $l->t('Addressbooks'); ?>" /></button>
		<form id="import_upload_form" action="<?php echo OCP\Util::linkTo('contacts', 'ajax/uploadimport.php'); ?>" method="post" enctype="multipart/form-data" target="import_upload_target">
			<input class="float" id="import_upload_start" type="file" accept="text/directory,text/vcard,text/x-vcard" name="importfile" />
			<input type="hidden" name="MAX_FILE_SIZE" value="<?php echo $_['uploadMaxFilesize'] ?>" id="max_upload">
		</form>
		<iframe name="import_upload_target" id='import_upload_target' src=""></iframe>
	</div>
</div>
<div id="rightcontent" class="rightcontent" data-id="<?php echo $_['id']; ?>">
	<?php
		if($_['has_contacts']) {
			echo $this->inc('part.contact');
		}
		else{
			echo $this->inc('part.no_contacts');
		}
	?>
	<div class="hidden popup" id="ninjahelp">
		<a class="close" tabindex="0" role="button" title="<?php echo $l->t('Close'); ?>"></a>
		<h2><?php echo $l->t('Keyboard shortcuts'); ?></h2>
		<div class="help-section">
			<h3><?php echo $l->t('Navigation'); ?></h3>
			<dl>
				<dt>j/Down</dt>
				<dd><?php echo $l->t('Next contact in list'); ?></dd>
				<dt>k/Up</dt>
				<dd><?php echo $l->t('Previous contact in list'); ?></dd>
				<dt>o</dt>
				<dd><?php echo $l->t('Expand/collapse current addressbook'); ?></dd>
				<dt>n/PageDown</dt>
				<dd><?php echo $l->t('Next addressbook'); ?></dd>
				<dt>p/PageUp</dt>
				<dd><?php echo $l->t('Previous addressbook'); ?></dd>
			</dl>
		</div>
		<div class="help-section">
			<h3><?php echo $l->t('Actions'); ?></h3>
			<dl>
				<dt>r</dt>
				<dd><?php echo $l->t('Refresh contacts list'); ?></dd>
				<dt>a</dt>
				<dd><?php echo $l->t('Add new contact'); ?></dd>
				<!-- dt>Shift-a</dt>
				<dd><?php echo $l->t('Add new addressbook'); ?></dd -->
				<dt>Shift-Delete</dt>
				<dd><?php echo $l->t('Delete current contact'); ?></dd>
			</dl>
		</div>
	</div>
</div>
<!-- Dialogs -->
<div id="dialog_holder"></div>
<!-- End of Dialogs -->
<menu type="context" id="addressbookmenu">
	<menuitem label="Delete" icon="<?php echo OCP\Util::imagePath('core', 'actions/delete.svg') ?>" onclick="alert('Really? ' + $(this).attr('data-id'))"></menuitem>
	<menuitem label="Rename" icon="<?php echo OCP\Util::imagePath('core', 'actions/rename.svg') ?>" onclick="alert('Can\'t do that')"></menuitem>
</menu>
