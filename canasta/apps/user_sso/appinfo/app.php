<?php

/**
* ownCloud - user_sso
*
* @author Manuel Delgado
* @copyright 2012 Manuel Delgado manuel.delgado@ucr.ac.cr
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

require_once 'apps/user_sso/lib/opensso.php';
require_once 'apps/user_sso/lib/hooks.php';
require_once 'apps/user_sso/user_sso.php';
OCP\Util::connectHook('OC_User', 'logout', 'OC_SSO_Hooks', 'logout');


//check if curl extension installed
if (!in_array ('curl', get_loaded_extensions())){
	OC_Log::write('OC_USER_SSO', 'CURL extension required', OC_Log::ERROR);
	return;
}

if (OCP\App::isEnabled('user_sso')) {
	if (!OCP\User::isLoggedIn()) {
		OCP\Util::addScript('user_sso', 'sso');
		OC_User::useBackend('SSO');
	} else {
		OC_User::useBackend('SSO');
	}
}

// Add settings page to navigation bar
/*OCP\App::registerAdmin('user_sso','settings');
$entry = array(
	'id' => 'user_sso_settings',
	'order'=>1,
	'href' => OCP\Util::linkTo( 'user_sso', 'settings.php' ),
	'name' => 'SSO'
);*/

