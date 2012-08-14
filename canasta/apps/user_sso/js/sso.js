/*Ideas: ejecutar el post a un modulo del app en lugar de llamar a index.php. Por ejemplo ?app=user_sso
Si retorna JSON success entonces es xq lo logueo y le da reload o lo redirige al SSO
Si retorna Error es xq no lo logró loguear
*/

function _COOKIE(q,c) {
	c = c ? c : document.cookie;
	var re = new RegExp(';?'+q+'(?:=([^;]*))?(?=;|$)','i');
	return (c=c.match(re)) ? (typeof c[1] == 'undefined' ? '' : decodeURIComponent(c[1])) : undefined;
} 

// Post with fake user/pass to SSO validation
$(document).ready(function(){
	var last_op = _COOKIE('sso_last');
	alert("last_op: " + last_op);
	if (last_op != 'loggedin' || last_op != 'error'){
		loginSSO(last_op);
	}
});

function loginSSO(last_op) {
	$.post('index.php',
	{   
		'authService' : 'sso',
		'user' : 'sso_user',
		'password' : 'sso_pass',
		'sectoken' : $('#sectoken').val()
	},
	function(data) {
		var sso_op = _COOKIE('sso_last');
		alert("sso_op: "+sso_op);
		if (data && data.status == 'error'){
			//error
			if (last_op != 'error' && (last_op != 'logout' && sso_op != 'error')){
				OC.dialogs.alert(t('user_sso', 'Please login first.'), t('user_sso', 'Failed to Login with SSO server'));
			}
			return;
		}
		if (data && data.status=='success'){
			//some data arrived
			if (data.redirect) {
				alert("Redirect Now!");
				window.location = data.redirect;
			}
			
			return;
		}
		if (data && data.indexOf('<html>') != -1) {
			if ((last_op == 'redirect' && sso_op != 'loggedin') || last_op == 'logout' || sso_op == 'error'){
				alert("No reload");
				return;
			}
			//window.document=data;
			window.location.reload();
			return;
		}

		// LogIn
		//window.location.reload();
	}
	);
}
