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

class OC_USER_SSO extends OC_User_Backend {

	// will be retrieved from LDAP server
	//protected $ldap_dc = false;

	// cache sso handler
	protected $sso;
	protected $_isSSORequest;
	

	public function __construct() {
		//TODO: pass config URL & server. May support several SSO methods depending on the class
		$this->_isSSORequest = @$_POST['authService'] == 'sso';
		$this->sso = new OpenSSO;
		error_log('[SSO] Termina construct');
	}

	/**
	 * @brief Check if the password is correct
	 * @param $uid The username
	 * @param $password The password
	 * @returns true/false
	 *
	 * Check if the password is correct without logging in the user
	 */
	public function checkPassword($uid, $password){
		error_log('[SSO] _isSSORequest '.$this->_isSSORequest.' uid '.$uid.' pass '.$password);
		if($this->_isSSORequest && $uid == 'sso_user' && $password == 'sso_pass'){
			error_log('[SSO] Entra checkPass');
			if ($this->sso->isSSOCookieAvailable()){
				error_log('[SSO] isSSOCookieAvailable');
				return $this->sso->checkSSOLogin(true);
			}else {
				error_log('[SSO] redirect');
				$this->sso->redirect(true);
				return false;
			}
		} else {
			error_log('[SSO] doSSOLogin');
			return $this->sso->doSSOLogin($uid, $password);
		}
	}

	/**
	 * @brief check if a user exists
	 * @param string $uid the username
	 * @return boolean
	 */
//Implementar esta que es llamada or isLoggedIn para saber si ya está logueado o no, de aca se verifica la cookie de ucr.ac.cr y se tiene que mappear el usuario desde el sso a un usuario en owncloud y a su vez supone que es un usuario de LDAP? 
// Hay q desloguar el usuario su no conincide!
	public function userExists($uid){
		if ($this->sso->isSSOCookieAvailable()){
			error_log('[SSO] userExists uid: '.$uid.' Y es? '.($this->sso->checkSSOLogin() == $uid));
			if ($this->sso->checkSSOLogin() == $uid) {
				error_log('[SSO] userExists true');
				return true;
			} else {
				// Log out user
				OC_User::logout();
			}
		}
		return false;
	}

}

?>
