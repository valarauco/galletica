<?php
/**
 * ownCloud - Addressbook
 *
 * @author Thomas Tanghus
 * @copyright 2012 Thomas Tanghus <thomas@tanghus.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU AFFERO GENERAL PUBLIC LICENSE
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU AFFERO GENERAL PUBLIC LICENSE for more details.
 *
 * You should have received a copy of the GNU Affero General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// Check if we are a user
OCP\JSON::checkLoggedIn();
OCP\JSON::checkAppEnabled('contacts');

$upload_max_filesize = OCP\Util::computerFileSize(ini_get('upload_max_filesize'));
$post_max_size = OCP\Util::computerFileSize(ini_get('post_max_size'));
$maxUploadFilesize = min($upload_max_filesize, $post_max_size);
$requesttoken = $_GET['requesttoken'];

$freeSpace=OC_Filesystem::free_space('/');
$freeSpace=max($freeSpace, 0);
$maxUploadFilesize = min($maxUploadFilesize, $freeSpace);
$adr_types = OC_Contacts_App::getTypesOfProperty('ADR');
$phone_types = OC_Contacts_App::getTypesOfProperty('TEL');
$email_types = OC_Contacts_App::getTypesOfProperty('EMAIL');

$tmpl = new OCP\Template('contacts', 'part.contact');
$tmpl->assign('uploadMaxFilesize', $maxUploadFilesize);
$tmpl->assign('uploadMaxHumanFilesize', OCP\Util::humanFileSize($maxUploadFilesize));
$tmpl->assign('adr_types', $adr_types);
$tmpl->assign('phone_types', $phone_types);
$tmpl->assign('email_types', $email_types);
$tmpl->assign('requesttoken', $requesttoken);
$tmpl->assign('id', '');
$page = $tmpl->fetchPage();

OCP\JSON::success(array('data' => array( 'page' => $page )));
