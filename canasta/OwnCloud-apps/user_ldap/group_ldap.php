<?php

/**
 * ownCloud – LDAP group backend
 *
 * @author Arthur Schiwon
 * @copyright 2012 Arthur Schiwon blizzz@owncloud.com
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

namespace OCA\user_ldap;

class GROUP_LDAP extends lib\Access implements \OCP\GroupInterface {
	protected $enabled = false;

	public function setConnector(lib\Connection &$connection) {
		parent::setConnector($connection);
		if(empty($this->connection->ldapGroupFilter) || empty($this->connection->ldapGroupMemberAssocAttr)) {
			$this->enabled = false;
		}
		$this->enabled = true;
	}

	/**
	 * @brief is user in group?
	 * @param $uid uid of the user
	 * @param $gid gid of the group
	 * @returns true/false
	 *
	 * Checks whether the user is member of a group or not.
	 */
	public function inGroup($uid, $gid) {
		error_log('[LDAP-G] inGroup: '.$uid. ' - '. $gid);
		if(!$this->enabled) {
			return false;
		}
		if($this->connection->isCached('inGroup'.$uid.':'.$gid)) {
			return $this->connection->getFromCache('inGroup'.$uid.':'.$gid);
		}
		$dn_user = $this->username2dn($uid);
		$dn_group = $this->groupname2dn($gid);
		// just in case
		if(!$dn_group || !$dn_user) {
			$this->connection->writeToCache('inGroup'.$uid.':'.$gid, false);
			return false;
		}
		//usually, LDAP attributes are said to be case insensitive. But there are exceptions of course.
		$members = $this->readAttribute($dn_group, $this->connection->ldapGroupMemberAssocAttr);
		if(!$members) {
			$this->connection->writeToCache('inGroup'.$uid.':'.$gid, false);
			return false;
		}

		//extra work if we don't get back user DNs
		//TODO: this can be done with one LDAP query
		if(strtolower($this->connection->ldapGroupMemberAssocAttr) == 'memberuid') {
			$dns = array();
			foreach($members as $mid) {
				$filter = str_replace('%uid', $mid, $this->connection->ldapLoginFilter);
				$ldap_users = $this->fetchListOfUsers($filter, 'dn');
				if(count($ldap_users) < 1) {
					continue;
				}
				$dns[] = $ldap_users[0];
			}
			$members = $dns;
		}

		$isInGroup = in_array($dn_user, $members);
		$this->connection->writeToCache('inGroup'.$uid.':'.$gid, $isInGroup);

		return $isInGroup;
	}

	/**
	 * @brief Get all groups a user belongs to
	 * @param $uid Name of the user
	 * @returns array with group names
	 *
	 * This function fetches all groups a user belongs to. It does not check
	 * if the user exists at all.
	 */
	public function getUserGroups($uid) {
		error_log('[LDAP-G] getUserGroups: '.$uid);
		if(!$this->enabled) {
			return array();
		}
		if($this->connection->isCached('getUserGroups'.$uid)) {
			return $this->connection->getFromCache('getUserGroups'.$uid);
		}
		$userDN = $this->username2dn($uid);
		if(!$userDN) {
			$this->connection->writeToCache('getUserGroups'.$uid, array());
			return array();
		}

		//uniqueMember takes DN, memberuid the uid, so we need to distinguish
		if((strtolower($this->connection->ldapGroupMemberAssocAttr) == 'uniquemember')
			|| (strtolower($this->connection->ldapGroupMemberAssocAttr) == 'member')
		) {
			$uid = $userDN;
		} else if(strtolower($this->connection->ldapGroupMemberAssocAttr) == 'memberuid') {
			$result = $this->readAttribute($userDN, 'uid');
			$uid = $result[0];
		} else {
			// just in case
			$uid = $userDN;
		}

		$filter = $this->combineFilterWithAnd(array(
			$this->connection->ldapGroupFilter,
			$this->connection->ldapGroupMemberAssocAttr.'='.$uid
		));
		$groups = $this->fetchListOfGroups($filter, array($this->connection->ldapGroupDisplayName,'dn'));
		$groups = array_unique($this->ownCloudGroupNames($groups), SORT_LOCALE_STRING);
		$this->connection->writeToCache('getUserGroups'.$uid, $groups);

		error_log('[LDAP-G] getUserGroups: $uid '.$uid.' OC_groups: '.implode(' ',$groups));
		error_log('[LDAP-G] getUserGroups: FIN!!');

		return $groups;
	}

	/**
	 * @brief get a list of all users in a group
	 * @returns array with user ids
	 */
	public function usersInGroup($gid, $search = '', $limit = -1, $offset = 0) {
		error_log('[LDAP-G] usersInGroup: '.$gid);
		if(!$this->enabled) {
			return array();
		}
		$this->groupSearch = $search;
		if($this->connection->isCached('usersInGroup'.$gid)) {
			$groupUsers = $this->connection->getFromCache('usersInGroup'.$gid);
			if(!empty($this->groupSearch)) {
				$groupUsers = array_filter($groupUsers, array($this, 'groupMatchesFilter'));
			}
			return array_slice($groupUsers, $offset, $limit);
		}

		$groupDN = $this->groupname2dn($gid);
		if(!$groupDN) {
			$this->connection->writeToCache('usersInGroup'.$gid, array());
			return array();
		}

		$members = $this->readAttribute($groupDN, $this->connection->ldapGroupMemberAssocAttr);
		error_log('[LDAP-G] usersInGroup: $members '.implode(' ',$members));
		if(!$members) {
			$this->connection->writeToCache('usersInGroup'.$gid, array());
			return array();
		}

		$result = array();
		$isMemberUid = (strtolower($this->connection->ldapGroupMemberAssocAttr) == 'memberuid');
		foreach($members as $member) {
			if($isMemberUid) {
				$filter = \OCP\Util::mb_str_replace('%uid', $member, $this->connection->ldapLoginFilter, 'UTF-8');
				$ldap_users = $this->fetchListOfUsers($filter, 'dn');
				if(count($ldap_users) < 1) {
					continue;
				}
				$result[] = $this->dn2username($ldap_users[0]);
				continue;
			} else {
				if($ocname = $this->dn2username($member)) {
					$result[] = $ocname;
				}
			}
		}
		error_log('[LDAP-G] usersInGroup: $result '.implode(' ',$result));
		if(!$isMemberUid) {
			$result = array_intersect($result, \OCP\User::getUsers());
			error_log('[LDAP-G] usersInGroup: OCP_users: '.implode(' ',\OCP\User::getUsers()).' $result22 '.implode(' ',$result));
		}
		$groupUsers = array_unique($result, SORT_LOCALE_STRING);
		$this->connection->writeToCache('usersInGroup'.$gid, $groupUsers);

		if(!empty($this->groupSearch)) {
			$groupUsers = array_filter($groupUsers, array($this, 'groupMatchesFilter'));
		}
		error_log('[LDAP-G] usersInGroup: $gid '.$gid.' OC_ucers: '.implode(' ',$groupUsers));
		error_log('[LDAP-G] usersInGroup: FIN!!');
		return array_slice($groupUsers, $offset, $limit);

	}

	/**
	 * @brief get a list of all groups
	 * @returns array with group names
	 *
	 * Returns a list with all groups
	 */
	public function getGroups($search = '', $limit = -1, $offset = 0) {
		error_log('[LDAP-G] getGroups');
		if(!$this->enabled) {
			return array();
		}

		if($this->connection->isCached('getGroups')) {
			$ldap_groups = $this->connection->getFromCache('getGroups');
		} else {
			$ldap_groups = $this->fetchListOfGroups($this->connection->ldapGroupFilter, array($this->connection->ldapGroupDisplayName, 'dn'));
			$ldap_groups = $this->ownCloudGroupNames($ldap_groups);
			$this->connection->writeToCache('getGroups', $ldap_groups);
		}
		$this->groupSearch = $search;
		if(!empty($this->groupSearch)) {
			$ldap_groups = array_filter($ldap_groups, array($this, 'groupMatchesFilter'));
		}
		error_log('[LDAP-G] getGroups: FIN!!');
		return array_slice($ldap_groups, $offset, $limit);
	}

	public function groupMatchesFilter($group) {
		return (strripos($group, $this->groupSearch) !== false);
	}

	/**
	 * check if a group exists
	 * @param string $gid
	 * @return bool
	 */
	public function groupExists($gid){
		error_log('[LDAP-G] getGroups: '.$gid);
		return in_array($gid, $this->getGroups());
	}

	/**
	* @brief Check if backend implements actions
	* @param $actions bitwise-or'ed actions
	* @returns boolean
	*
	* Returns the supported actions as int to be
	* compared with OC_USER_BACKEND_CREATE_USER etc.
	*/
	public function implementsActions($actions) {
		//always returns false, because possible actions are modifying actions. We do not write to LDAP, at least for now.
		return false;
	}
}