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

// Class to manage OpenSSO/OpenAM SSO login
class OpenSSO{

	protected $ssoURL = 'https://sso.ucr.ac.cr/';
	protected $ssoDomain = 'opensso/';
	protected $ssoRootDomain = 'ucr.ac.cr';
	protected $ssoLogoutURL = 'identity/logout';
	protected $ssoAuthURL = 'identity/authenticate';
	protected $ssoValidURL = 'identity/isTokenValid';
	protected $ssoAtributesURL = 'identity/attributes';
	protected $ssoCookieName = 'iPlanetDirectoryPro';


	function OpenSSO(){
		//TODO: Set parameters to URLS
	}

	function isSSOCookieAvailable() {
		//error_log('[SSO] isSSOCookieAvailable cookie?: '. isset($_COOKIE) . ' y iPlanetDirectoryPro?:' . array_key_exists($this->ssoCookieName, $_COOKIE));
		return isset($_COOKIE) && array_key_exists($this->ssoCookieName, $_COOKIE);
	}

  function getSSOCookie() {
      if (!$this->isSSOCookieAvailable()) {
          return '';
      }
			//error_log('[SSO] getSSOCookie: '.$_COOKIE[$this->ssoCookieName]);
			$cookie = $this->cookieSanitize($_COOKIE[$this->ssoCookieName]);
      return $cookie;
  }

	function checkSSOLogin($isJSON = false) {
		error_log('[SSO] checkSSOLogin');
		$cookie = $this->getSSOCookie();
		if ($this->isValidCookie($cookie)){
			$uid = $this->getSSOAttribute($cookie, 'uid');
			if ($uid != '') {
				setCookie('sso_last', 'loggedin');
				return $uid;
			}
		}
		setCookie('sso_last', 'error');
		if ($isJSON) {
				OCP\JSON::error(array());
				exit();
		} else {
			return false;
		}
	}

	function isValidCookie($cookie){
		$response = $this->CURL_Request(
				$this->ssoURL.$this->ssoDomain.$this->ssoValidURL,
				"POST", 
				array("tokenid" => urlencode($cookie)));
		//error_log('[SSO] isValidCookie response: '.$response);
		list($key, $value) = explode("=", $response, 2);
		return trim($key) == 'boolean' && trim($value) == 'true';
	}

	function getSSOAttribute($cookie, $attr=''){
		$response = $this->CURL_Request(
				$this->ssoURL.$this->ssoDomain.$this->ssoAtributesURL,
				"POST", 
				array("subjectid" => urlencode($cookie)));
		//error_log('[SSO] getSSOAttribute attribute: '.$attr.' response: '.$response);
		$attributes = $this->splitSSOResponse($response);
		if (isset($attributes["userdetails.token.id"]) && $attributes["userdetails.token.id"] == $cookie) {
			//error_log('[SSO] getSSOAttribute COOKIE!');
			if ($attr != '' && isset($attributes[$attr])) {
				error_log('[SSO] getSSOAttribute value '.$attributes[$attr]);
				return strtolower($attributes[$attr]);
			}
			return $attributes;
		}
		return '';
	}

	function doSSOLogin($uid, $password){
		if ($uid != '' && $password != '') {
			error_log('[SSO] Dologin pass encoded '.urlencode($password));
			$cookie = $this->getSSOCookie();
			$response = $this->CURL_Request(
					$this->ssoURL.$this->ssoDomain.$this->ssoAuthURL,
					"POST", 
					array("username" => $uid, "password" => urlencode($password) )/* ,
					array($this->ssoCookieName => urlencode($cookie))*/);
			error_log('[SSO] Dologin response \n'.$response);
			$r = $this->splitSSOResponse($response);
			if(isset($r['token.id'])) {
				error_log('[SSO] Dologin tokein id \n'.$r['token.id']);
				setcookie($this->ssoCookieName, $r['token.id'], 0,'/', $this->ssoRootDomain);
			}
			return $this->checkSSOLogin();
		}
		return false;
	}

	function doSSOLogout(){
		$cookie = $this->getSSOCookie();
		if ($this->isValidCookie($cookie)){
			$response = $this->CURL_Request(
					$this->ssoURL.$this->ssoDomain.$this->ssoLogoutURL,
					"POST", 
					array("subjectid" => urlencode($cookie)));
			error_log('[SSO] logout response \n'.$response);
			setCookie('sso_last', 'logout');
			if ($this->isSSOCookieAvailable()){
				error_log('[SSO] logout unsetting cookies');
				unset($_COOKIE[$this->ssoCookieName]);
				setcookie($this->ssoCookieName, NULL, -1);
				return true;
			}
		}
	}

	function redirect($isJSON = false){
		$redirect_to = $this->getRedirectURL();
		error_log('[SSO] RedirectURL: '.$redirect_to);
		
		setCookie('sso_last', 'redirect');
		if ($isJSON) {
			OCP\JSON::success(array('redirect' => $redirect_to));
		} else {
			OCP\Response::redirect($redirect_to);
		}
		exit();
	}

	function getRedirectURL() {
		return $this->ssoURL.$this->ssoDomain.'?goto=http://' . $_SERVER["HTTP_HOST"] . OC::$WEBROOT;
	}

	// Creates an array based on openSSO response
	function splitSSOResponse($response) {
		$r = array();
		$response = explode("\n", $response);
		$ssokey = "";
		foreach($response as $line) {
			$line = trim($line);
			if ($line != "") {
				list($key, $value) = explode("=", $line, 2);
				//$r[trim($key)] = trim($value);
				if( trim($key) == "userdetails.attribute.name" ){
					$ssokey = trim($value);
					continue;
				} else if( trim($key) == "userdetails.attribute.value" && $ssokey != "" ){
					$r[$ssokey] = $value;
				} else {
					$r[trim($key)] = trim($value);
				}
			}
			$ssokey = "";
		}
	 	return $r;
	} 

	//Repare problem with space character
	function cookieSanitize($cookie) {
		return str_replace(' ', '+', $cookie);
	}


// Must be separeted?
	function CURL_Request($url, $method="GET", $params = "", $cookie = "") { // Remember, SSL MUST BE SUPPORTED
		if (is_array($params)) $params = $this->array2url($params);
		if (is_array($cookie)) $cookie = $this->array2cookie($cookie);
		//error_log('[SSO] CURL_Request url: '.$url.' method: '. $method. ' params: '. $params .' cookie: '.$cookie = "");
		$curl = curl_init($url . ($params != "" ? "?" . $params : ""));
		curl_setopt($curl, CURLOPT_FOLLOWLOCATION, true);
		curl_setopt($curl, CURLOPT_HEADER, false);
		curl_setopt($curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_setopt($curl, CURLOPT_HTTPGET, ($method == "GET"));
		curl_setopt($curl, CURLOPT_POST, ($method == "POST"));
		if ($cookie != "") curl_setopt($curl, CURLOPT_COOKIE, $cookie);
		if ($method == "POST") curl_setopt($curl, CURLOPT_POSTFIELDS, $params);
		curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
		$response = curl_exec($curl);
		
		//TODO: catch errors
		/*if (curl_errno($curl) == 0){
			$response;
		}else{
			$this->ErrorStore('OPENID_CURL', curl_error($curl));
		}*/
		return $response;
	}

	function array2url($arr){ // converts associated array to URL Query String
		if (!is_array($arr)){
			return false;
		}
		$query = '';
		foreach($arr as $key => $value){
			$query .= $key . "=" . $value . "&";
		}
		return $query;
	}

	function array2cookie($arr){ // converts associated array to cookie String ("name=value;name2=value2")
		if (!is_array($arr)){
			return false;
		}
		$query = '';
		foreach($arr as $key => $value){
			$query .= $key . "=" . $value . ";";
		}
		return $query;
	}

}

?>
