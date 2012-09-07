<?php

/**
* ownCloud - files_antivirus
*
* @author Manuel Deglado
* @copyright 2012 Manuel Deglado manuel.delgado@ucr.ac.cr
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
//TODO: Hide Host/port and path depending on mode

$params = array('av_mode', 'av_host', 'av_port', 'av_path',)

if($_POST){
	foreach($params as $param){
		if(isset($_POST[$param])){
			OCP\Config::setAppValue('files_antivirus', $param, $_POST[$param]);
		}
	}
}

// fill template
$tmpl = new OC_Template( 'files_antivirus', 'settings');
foreach($params as $param){
		$value = OCP\Config::getAppValue('files_antivirus', $param,'');
		$tmpl->assign($param, $value);
}
//Default
$tmpl->assign( 'av_path', OCP\Config::getAppValue('files_antivirus', 'av_mode', 'executable'));
$tmpl->assign( 'av_path', OCP\Config::getAppValue('files_antivirus', 'av_path', '/usr/bin/clamscan'));

return $tmpl->fetchPage();
