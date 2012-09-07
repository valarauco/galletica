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

// The file was not checked (e.g. because the AV daemon wasn't running).
define('CLAMAV_SCANRESULT_UNCHECKED', -1);
// The file was checked and found to be clean.
define('CLAMAV_SCANRESULT_CLEAN', 0);
// The file was checked and found to be infected.
define('CLAMAV_SCANRESULT_INFECTED', 1);

class OC_Files_Antivirus {
	/*private $av_mode;
	private $av_host;
	private $av_port;
	private $av_path;
	
	public function init(){
		$this->readConfiguration();	
	}
	
	private function readConfiguration(){
		\OCP\Util::writeLog('files_antivirus','Reading the configuration', \OCP\Util::DEBUG);
		$this->$av_mode = \OCP\Config::getAppValue('files_antivirus', 'av_mode', 'executable');
		$this->$av_host = \OCP\Config::getAppValue('files_antivirus', 'av_host', '');
		$this->$av_port = \OCP\Config::getAppValue('files_antivirus', 'av_port', '');
		$this->$av_path = \OCP\Config::getAppValue('files_antivirus', 'av_path', '/usr/bin/clamscan');
	
	}*/
	
	function av_scan($filepath) {
		$result = clamav_scan($filepath);

/*TODO: throw an error and/or send the mail
		if($result == CLAMAV_SCANRESULT_INFECTED) {
		  form_set_error($form_element, t('A virus has been detected in the file.  The file will not be accepted.'));
		}
		elseif ($result == CLAMAV_SCANRESULT_UNCHECKED && variable_get('clamav_unchecked_files', CLAMAV_DEFAULT_UNCHECKED) == CLAMAV_BLOCK_UNCHECKED) {
		  form_set_error($form_element, t('The anti-virus scanner was not able to check the file.  The file cannot be uploaded.'));
  }
  */
	}
	
	function clamav_scan($filepath) {
		$av_mode = \OCP\Config::getAppValue('files_antivirus', 'av_mode', 'executable');
		switch($av_mode) {
			case 'daemon':
				return _clamav_scan_via_daemon($filepath);
			case 'executable':
				return _clamav_scan_via_exec($filepath);
		}
  }
  
	function _clamav_scan_via_daemon($filepath) {
		$av_host = \OCP\Config::getAppValue('files_antivirus', 'av_host', '');
		$av_port = \OCP\Config::getAppValue('files_antivirus', 'av_port', '');
		
		// try to open a socket to clamav
		$handler = ($av_host && $av_port) ? @fsockopen($av_host, $av_port) : false;

		if(!$handler) {
			\OCP\Util::writeLog('files_antivirus','The clamav module is not configured for daemon mode.', \OCP\Util::ERROR);
		  return false;
		}

		// request scan from the daemon
		// TODO: check for remote scanning!
		fwrite($handler, "SCAN {$filepath}\n");
		$response = fgets($handler);
		fclose($handler);

		// clamd returns a string response in the format:
		// filename: OK
		// filename: <name of virus> FOUND
		// filename: <error string> ERROR

		if (preg_match('/.*: OK$/', $response)) {
		  return CLAMAV_SCANRESULT_CLEAN;
		}
		elseif (preg_match('/.*: (.*) FOUND$/', $response, $matches)) {
		  $virus_name = $matches[1];
		  \OCP\Util::writeLog('files_antivirus','Virus detected in file.  Clamav reported the virus: '.$virus_name, \OCP\Util::WARN);
		  return CLAMAV_SCANRESULT_INFECTED;
		}
		else {
		  // try to extract the error message from the response.
		  preg_match('/.*: (.*) ERROR$/', $response, $matches);
		  $error_string = $matches[1]; // the error message given by the daemon
		  \OCP\Util::writeLog('files_antivirus','File could not be scanned.  Clamscan reported: '.$error_string, \OCP\Util::WARN);
		  return CLAMAV_SCANRESULT_UNCHECKED;
		}
	}
	
	function _clamav_scan_via_exec($filepath) {
		// get the path to the executable
		$av_path = \OCP\Config::getAppValue('files_antivirus', 'av_path', '/usr/bin/clamscan');

		// check that the executable is available
		if (!file_exists($av_path)) {
			\OCP\Util::writeLog('files_antivirus','The clamscan executable could not be found at '.$av_path, \OCP\Util::ERROR);
			return CLAMAV_SCANRESULT_UNCHECKED;
		}

		// using 2>&1 to grab the full command-line output.
		$cmd = escapeshellcmd($executable) .' '. escapeshellarg($filepath) . ' 2>&1';
		exec($cmd, $output, $result);


		/**
		 * clamscan return values (documented from man clamscan)
		 *  0 : No virus found.
		 *  1 : Virus(es) found.
		 *  X : Error.
		 * TODO: add errors?
		 */
		switch($result) {
			case 0:
			  return CLAMAV_SCANRESULT_CLEAN;

			case 1:
			  // pass each line of the exec output through checkplain.
			  // The t operator ! is used instead of @, because <br /> tags are being added.
			  foreach($output as $key => $line) {
			    $output[$key] = check_plain($line);
			  }
			  \OCP\Util::writeLog('files_antivirus','Virus detected in file.  Clamscan reported: '.implode(', ', $output), \OCP\Util::WARN);
			  return CLAMAV_SCANRESULT_INFECTED;

			default:
			  $descriptions = array(
			    40 => "Unknown option passed.",
			    50 => "Database initialization error.",
			    52 => "Not supported file type.",
			    53 => "Can't open directory.",
			    54 => "Can't open file. (ofm)",
			    55 => "Error reading file. (ofm)",
			    56 => "Can't stat input file / directory.",
			    57 => "Can't get absolute path name of current working directory.",
			    58 => "I/O error, please check your file system.",
			    62 => "Can't initialize logger.",
			    63 => "Can't create temporary files/directories (check permissions).",
			    64 => "Can't write to temporary directory (please specify another one).",
			    70 => "Can't allocate memory (calloc).",
			    71 => "Can't allocate memory (malloc).",
			  );
			  $description = (array_key_exists($result, $descriptions)) ? $descriptions[$result] : 'unknown error';

				\OCP\Util::writeLog('files_antivirus','File could not be scanned.  Clamscan reported: '.$result, \OCP\Util::WARN);
		  	return CLAMAV_SCANRESULT_UNCHECKED;
		}
	}

}
