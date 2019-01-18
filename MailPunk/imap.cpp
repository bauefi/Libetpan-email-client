/* Implementation file "imap.cpp" */

/* Author: Ulrik Stig Hansen
   Program last changed: 13th December 2018 */

#include "imap.hpp"
#include <string>
using namespace std;
using namespace IMAP;

/* MESSAGE CLASS */

void Message::to_string(clist* list) {
  for (clistiter* cur = clist_begin(list); cur != NULL; cur = clist_next(cur)){
    auto address = (mailimap_address*) clist_content(cur);
    if(address -> ad_mailbox_name && address -> ad_host_name) {
      from += address -> ad_mailbox_name;
      from += '@';
      from += address ->ad_host_name;
    }
    else {
      from += "UNKOWN SENDER";}
  }
}

std::string Message::getBody() {

  auto set = (mailimap_set*) mailimap_set_new_single(uid);
  auto fetch_type = (mailimap_fetch_type*) mailimap_fetch_type_new_fetch_att_list_empty();
  auto section = mailimap_section_new(NULL);
  auto fetch_att = (mailimap_fetch_att*) mailimap_fetch_att_new_body_section(section);
  size_t msg_len;
  string msg_content;
  clist * fetch_result;

  mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);

  r = mailimap_uid_fetch(imap, set, fetch_type, &fetch_result);
  check_error(r, "could not fetch");

  msg_content = get_msg_content(fetch_result, &msg_len);

  mailimap_fetch_list_free(fetch_result);
  mailimap_fetch_type_free(fetch_type);
  mailimap_set_free(set);

  return msg_content;
}

std::string Message::get_msg_content(clist * fetch_result, size_t * p_msg_size) {

  clistiter * cur;

	for(cur = clist_begin(fetch_result) ; cur != NULL ; cur = clist_next(cur)) {
    auto msg_att = (struct mailimap_msg_att*)clist_content(cur);
		size_t msg_size;
		string msg_content;
		msg_content = get_msg_att_msg_content(msg_att, &msg_size);
    if (msg_content.empty()) {
			continue;
		}
		* p_msg_size = msg_size;
		return msg_content;
	}
	return NULL;
}

std::string Message::get_msg_att_msg_content(struct mailimap_msg_att * msg_att, size_t * p_msg_size) {

  clistiter * cur;

  for(cur = clist_begin(msg_att->att_list) ; cur != NULL ; cur = clist_next(cur)) {
    auto item = (struct mailimap_msg_att_item*)clist_content(cur);
    if(item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
      continue;
    }

    if(item -> att_data.att_static -> att_type == MAILIMAP_MSG_ATT_ENVELOPE) {

      if(item -> att_data.att_static -> att_data.att_env -> env_from -> frm_list)
        to_string(item -> att_data.att_static -> att_data.att_env -> env_from -> frm_list);

      if(item -> att_data.att_static -> att_data.att_env -> env_subject) {
        return item -> att_data.att_static -> att_data.att_env -> env_subject;
      }

    } else {

      if(item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_BODY_SECTION) {
        continue;
      }
      *p_msg_size = item->att_data.att_static->att_data.att_body_section->sec_length;
      return item->att_data.att_static->att_data.att_body_section->sec_body_part;

    }


  }
  return NULL;
}

std::string Message::getField(std::string fieldname) {

  auto set = (mailimap_set*) mailimap_set_new_single(uid);
  auto fetch_type = (mailimap_fetch_type*) mailimap_fetch_type_new_fetch_att_list_empty();
  auto fetch_att = (mailimap_fetch_att*) mailimap_fetch_att_new_envelope();
  size_t msg_len;
  string field;
  clist * fetch_result;

  mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);

  r = mailimap_uid_fetch(imap, set, fetch_type, &fetch_result);
  check_error(r, "could not fetch");

  field = get_msg_content(fetch_result, &msg_len);

  mailimap_fetch_list_free(fetch_result);
  mailimap_fetch_type_free(fetch_type);
  mailimap_set_free(set);

  if(fieldname == "From") {
    return from;
  }

  if(fieldname == "Subject") {
    return field;
  }

}


void Message::deleteFromMailbox(){
  struct mailimap_set* set = mailimap_set_new_single(uid);
  struct mailimap_flag_list* flag_list = mailimap_flag_list_new_empty();
  mailimap_flag_list_add(flag_list, mailimap_flag_new_deleted());
  struct mailimap_store_att_flags* att_flags = mailimap_store_att_flags_new_set_flags(flag_list);
  mailimap_uid_store(imap, set, att_flags);
  mailimap_expunge(imap);
  mailimap_set_free(set);
  mailimap_store_att_flags_free(att_flags);

  for(int i = 0; messages[i] != nullptr; i++) {
    if(this == messages[i])
      continue;

    delete messages[i];
  }
  delete [] messages;

  updateUI();

  delete this;
}

/* SESSION CLASS */

int Session::numberOfMessages() {
  char mb [] = "INBOX";
  mailimap_mailbox_data_status* result;
  int r;

  auto status_att_list = mailimap_status_att_list_new_empty();

  r = mailimap_status_att_list_add(status_att_list, MAILIMAP_STATUS_ATT_MESSAGES);
  check_error(r, "Could not add attribute");

  r = mailimap_status(imap, mb, status_att_list, &result);
  check_error(r, "Could not get message number");

  auto status_info = (mailimap_status_info*)clist_content(clist_begin(result->st_info_list));
  int num_messages = status_info -> st_value;

  mailimap_mailbox_data_status_free(result);
  mailimap_status_att_list_free(status_att_list);

  return num_messages;
}

Message** Session::getMessages() {

  int num_messages = numberOfMessages();
  messages = new Message*[num_messages + 1];

  if(num_messages == 0) {
    messages[0] = nullptr;

    return messages;
  }

  auto set = (mailimap_set*) mailimap_set_new_interval(1, 0);
  auto fetch_type = (mailimap_fetch_type*) mailimap_fetch_type_new_fetch_att_list_empty();
  auto fetch_att = (mailimap_fetch_att*) mailimap_fetch_att_new_uid();
  clist * fetch_result;
  clistiter * cur;

  mailimap_fetch_type_new_fetch_att_list_add(fetch_type, fetch_att);

  r = mailimap_fetch(imap, set, fetch_type, &fetch_result);
  check_error(r, "could not fetch");

  int i = 0;
  for(cur = clist_begin(fetch_result); cur != NULL; cur = clist_next(cur)) {
    auto msg_att = (struct mailimap_msg_att*)clist_content(cur);
		uint32_t uid = get_uid(msg_att);
    messages[i] = new Message(imap, uid, updateUI, messages);
    i++;
	 }

   messages[i] = nullptr;

   mailimap_fetch_list_free(fetch_result);
   mailimap_fetch_type_free(fetch_type);
   mailimap_set_free(set);

   return messages;
}

void Session::connect(std::string const& server, size_t port) {
  r = mailimap_socket_connect(imap, server.c_str(), port);
  cerr << "connect: " << r << endl;
	check_error(r, "could not connect to server");
}

void Session::login(std::string const& userid, std::string const& password) {
  r = mailimap_login(imap, userid.c_str(), password.c_str());
	check_error(r, "could not login");
}

void Session::selectMailbox(std::string const& mailbox) {
  r = mailimap_select(imap, "INBOX");
	check_error(r, "could not select INBOX");
}

uint32_t Session::get_uid(struct mailimap_msg_att * msg_att) {

	clistiter * cur;

  for(cur = clist_begin(msg_att->att_list) ; cur != NULL ; cur = clist_next(cur)) {
	  auto item = (struct mailimap_msg_att_item*)clist_content(cur);
    if (item->att_type != MAILIMAP_MSG_ATT_ITEM_STATIC) {
			continue;
		}
		if (item->att_data.att_static->att_type != MAILIMAP_MSG_ATT_UID) {
			continue;
		}
		return item->att_data.att_static->att_data.att_uid;
	}

	return 0;
}
