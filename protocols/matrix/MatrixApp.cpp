/*
 * Copyright 2021, Jaidyn Levesque <jadedctrl@teknik.io>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "MatrixApp.h"

#include <iostream>
#include <string>

#include <Catalog.h>
#include <MessageRunner.h>
#include <Roster.h>

#include <ChatProtocolMessages.h>
#include <Flags.h>
#include <UserStatus.h>

#include "Matrix.h"
#include "MatrixMessages.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MatrixApp"


std::shared_ptr<mtx::http::Client> client = nullptr;
MatrixApp* m_app = NULL;


int
main(int arc, char** argv)
{
	MatrixApp app;
	app.Run();
	return 0;
}


MatrixApp::MatrixApp()
	:
	BApplication(MATRIX_SIGNATURE),
	fUser(NULL),
	fPassword(NULL),
	fInitStatus(B_NOT_INITIALIZED),
	fProtoThread(-1),
	fConnected(false)
{
	new BMessageRunner(this, new BMessage(CHECK_APP), 10000000, -1);
}


void
MatrixApp::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MATRIX_REGISTER_ACCOUNT:
		{
			int64 thread_id;
			if (msg->FindInt64("thread_id", &thread_id) != B_OK)
				break;

			fProtoThread = thread_id;
			fUser = msg->FindString("username");
			fSession = msg->GetString("session", "Chat-O-Matic [Haiku]");
			fServer = msg->FindString("server");
			fPassword = msg->FindString("password");

			app_info info;
			GetAppInfo(&info);
			BMessage registerApp(MATRIX_ACCOUNT_REGISTERED);
			registerApp.AddInt64("team_id", info.team);
			SendMessage(registerApp);

			Connect();
			break;
		}
		case CHECK_APP:
		{
			BRoster roster;
			if (roster.IsRunning(APP_SIGNATURE) == false)
				Quit();
			break;
		}
		case IM_MESSAGE:
		{
			ImMessage(msg);
			break;
		}
		default:
			BApplication::MessageReceived(msg);
	}
}


void
MatrixApp::ImMessage(BMessage* msg)
{
	int32 im_what = msg->GetInt32("im_what", -1);
	switch (im_what) {
		case IM_SET_OWN_STATUS:
		{
			int32 status = msg->GetInt32("status", -1);
			if (status == STATUS_OFFLINE)
				Disconnect();
			break;
		}
		case IM_SEND_MESSAGE:
		{
			const char* chat_id = msg->FindString("chat_id");
			const char* body = msg->FindString("body");
			if (chat_id != NULL && body != NULL)
				RoomSendMessage(chat_id, body);
			break;
		}
		case IM_JOIN_ROOM:
		{
			const char* chat_id = msg->FindString("chat_id");
			if (chat_id != NULL)
				RoomJoin(chat_id);
			break;
		}
		case IM_LEAVE_ROOM:
		{
			const char* chat_id = msg->FindString("chat_id");
			if (chat_id != NULL)
				RoomLeave(chat_id);
			break;
		}
		case IM_GET_ROOM_PARTICIPANTS:
		{
			const char* chat_id = msg->FindString("chat_id");
			if (chat_id != NULL)
				RoomGetParticipants(chat_id);
			break;
		}
		case IM_ROOM_INVITE_ACCEPT:
		{
			const char* chat_id = msg->FindString("chat_id");
			if (chat_id != NULL)
				RoomJoin(chat_id);
			break;
		}
		case IM_ROOM_INVITE_REFUSE:
		{
			const char* chat_id = msg->FindString("chat_id");
			if (chat_id != NULL)
				RoomLeave(chat_id);
			break;
		}
		case IM_SET_OWN_NICKNAME:
		{
			const char* user_name = msg->FindString("user_name");
			if (user_name != NULL)
				SetNickname(user_name);
			break;
		}
		default: {
			std::cout << "Unhandled message for Matrix:\n";
			msg->PrintToStream();
		}
	}
}


void
MatrixApp::Connect()
{
	try {
		client = std::make_shared<mtx::http::Client>(fServer.String());
	}
	catch (std::exception &e) {
		SendError("Unable to initialize Matrix client.", {}, true);
		return;
	}

	client->set_device_id(fSession.String());

	BMessage progress(IM_MESSAGE);
	progress.AddInt32("im_what", IM_PROGRESS);
	progress.AddString("message", B_TRANSLATE("Contacting homeserver..."));
	SendMessage(progress);

	client->login(fUser.String(), fPassword.String(),
		[this](const mtx::responses::Login &res, mtx::http::RequestErr err)
		{
			if (err) {
				SendError("Error occured during login, please try again.", err,
					true);
				fInitStatus = B_ERROR;
				fConnected = false;
				return;
			}
			client->set_access_token(res.access_token);
			fInitStatus = B_OK;
			fConnected = true;
			StartLoop();
		});
}


void
MatrixApp::Disconnect()
{
	fConnected = false;
	client = nullptr;

	BMessage status(IM_MESSAGE);
	status.AddInt32("im_what", IM_OWN_STATUS_SET);
	status.AddInt32("status", (int32)STATUS_OFFLINE);
	SendMessage(status);
}


void
MatrixApp::StartLoop()
{
	if (!client)
		return;

	BMessage status(IM_MESSAGE);
	status.AddInt32("im_what", IM_OWN_STATUS_SET);
	status.AddInt32("status", (int32)STATUS_ONLINE);
	SendMessage(status);

	BMessage syncStatus(IM_MESSAGE);
	syncStatus.AddInt32("im_what", IM_MESSAGE_RECEIVED);
	syncStatus.AddString("user_name", APP_NAME);
	syncStatus.AddString("body", B_TRANSLATE("Synchronizing with Matrix server. Please wait..."));
	SendMessage(syncStatus);

	client->get_profile(client->user_id().to_string(),
		[this](const mtx::responses::Profile &res, mtx::http::RequestErr err)
		{
			if (err) {
				print_error(err, "Failed getting own info after login…");
				StartLoop();
				snooze(1000000);
				return;
			}
			BMessage ready(IM_MESSAGE);
			ready.AddInt32("im_what", IM_PROTOCOL_READY);
			SendMessage(ready);

			BMessage init(IM_MESSAGE);
			init.AddInt32("im_what", IM_OWN_CONTACT_INFO);
			init.AddString("user_id", client->user_id().to_string().c_str());
			init.AddString("user_name", res.display_name.c_str());
			SendMessage(init);

			mtx::http::SyncOpts opts;
			opts.timeout = 0;
			client->sync(opts, &initial_sync_handler);
		});
}


void
MatrixApp::SendMessage(BMessage msg)
{
	if (fProtoThread <= 0)
		return;

	ssize_t size = msg.FlattenedSize();
	char buffer[size];

	send_data(fProtoThread, size, NULL, 0);
	msg.Flatten(buffer, size);
	send_data(fProtoThread, 0, buffer, size);
}


void
MatrixApp::SendError(const char* message, mtx::http::RequestErr err,
	bool fatal)
{
	if (err)
		print_error(err);

	BString detail;
	if (err && !err->matrix_error.error.empty())
		detail << err->matrix_error.error.c_str();

	BMessage error(IM_ERROR);
	error.AddString("error", message);
	error.AddString("detail", detail);
	SendMessage(error);

	if (fatal == true) {
		BMessage disable(IM_MESSAGE);
		disable.AddInt32("im_what", IM_PROTOCOL_DISABLE);
		SendMessage(disable);
	}
}


void
MatrixApp::RoomSendMessage(const char* chat_id, const char* body)
{
	if (!fConnected || !client || chat_id == NULL || body == NULL)
		return;

	std::string id(chat_id);
	std::string message(body);

	mtx::events::msg::Text text;
	text.body = message;
	client->send_room_message<mtx::events::msg::Text>(id, text,
		[this, id, message](const mtx::responses::EventId &res,
			mtx::http::RequestErr err)
		{
			if (err) {
				SendError("Failed to send message.", err);
				return;
			}

			BMessage sent(IM_MESSAGE);
			sent.AddInt32("im_what", IM_MESSAGE_SENT);
			sent.AddString("chat_id", id.c_str());
			sent.AddString("body", message.c_str());
			SendMessage(sent);
		});
}


void
MatrixApp::RoomJoin(const char* chat_id)
{
	if (!fConnected || !client || chat_id == NULL)
		return;

	std::string id(chat_id);
	client->join_room(id,
		[this](const mtx::responses::RoomId &res,
			mtx::http::RequestErr err)
		{
			if (err) {
				SendError("Failed to join room.", err);
				return;
			}
			// The room will be confirmed through the next sync
			// (rooms.join), which emits IM_ROOM_JOINED.
		});
}


void
MatrixApp::RoomLeave(const char* chat_id)
{
	if (!fConnected || !client || chat_id == NULL)
		return;

	std::string id(chat_id);
	client->leave_room(id,
		[this, chat_id](const mtx::responses::Empty &res,
			mtx::http::RequestErr err)
		{
			if (err) {
				SendError("Failed to leave room.", err);
				return;
			}

			BMessage left(IM_MESSAGE);
			left.AddInt32("im_what", IM_ROOM_LEFT);
			left.AddString("chat_id", chat_id);
			SendMessage(left);

			if (fRoomList.HasString(BString(chat_id)) == true)
				fRoomList.Remove(BString(chat_id));
		});
}


void
MatrixApp::RoomGetParticipants(const char* chat_id)
{
	if (!fConnected || !client || chat_id == NULL)
		return;

	std::string id(chat_id);
	client->members(id,
		[this, chat_id](const mtx::responses::Members& members,
			mtx::http::RequestErr err)
		{
			if (err) {
				SendError("Failed to fetch room participants.", err);
				return;
			}

			BMessage msg(IM_MESSAGE);
			msg.AddInt32("im_what", IM_ROOM_PARTICIPANTS);
			msg.AddString("chat_id", chat_id);
			for (const auto &chunk : members.chunk) {
				if (chunk.content.membership
						!= mtx::events::state::Membership::Join)
					continue;
				msg.AddString("user_id", chunk.state_key.c_str());
				if (!chunk.content.display_name.empty())
					msg.AddString("user_name", chunk.content.display_name.c_str());
			}
			SendMessage(msg);
		});
}


void
MatrixApp::SetNickname(const char* display_name)
{
	if (!fConnected || !client || display_name == NULL)
		return;

	std::string name(display_name);
	client->set_displayname(name,
		[this, name](mtx::http::RequestErr err)
		{
			if (err) {
				SendError("Failed to set display name.", err);
				return;
			}

			BMessage nick(IM_MESSAGE);
			nick.AddInt32("im_what", IM_OWN_NICKNAME_SET);
			nick.AddString("user_name", name.c_str());
			SendMessage(nick);
		});
}


void
print_error(mtx::http::RequestErr err, const char* message)
{
	if (message != NULL)
		std::cerr << message << " ― ";
	if (!err) {
		std::cerr << "Unknown network error" << std::endl;
		return;
	}
	std::cerr << err->status_code << " : " << err->error_code;
	if (!err->matrix_error.error.empty())
		std::cerr << " : " << err->matrix_error.error;
	std::cerr << std::endl;
}


void
initial_sync_handler(const mtx::responses::Sync &res, mtx::http::RequestErr err)
{
	mtx::http::SyncOpts opts;

	if (!client)
		return;

	if (err) {
		print_error(err, "Error occured during initial sync. Retrying…");
		if ((int)err->status_code != 200) {
			opts.timeout = 0;
			client->sync(opts, &initial_sync_handler);
		}
		return;
	}

	room_sync(res.rooms);
	invite_sync(res.rooms.invite);

	opts.since = res.next_batch;
	client->set_next_batch_token(res.next_batch);
	client->sync(opts, &sync_handler);
}


// Callback to executed after a /sync request completes.
void
sync_handler(const mtx::responses::Sync &res, mtx::http::RequestErr err)
{
	mtx::http::SyncOpts opts;

	if (!client)
		return;

	if (err) {
		print_error(err, "Error occured during sync. Retrying…");
		opts.since = client->next_batch_token();
		client->sync(opts, &sync_handler);
		return;
	}

	room_sync(res.rooms);
	invite_sync(res.rooms.invite);

	opts.since = res.next_batch;
	client->set_next_batch_token(res.next_batch);
	client->sync(opts, &sync_handler);
}


void
invite_sync(std::map<std::string, mtx::responses::InvitedRoom> invites)
{
	MatrixApp* app = (MatrixApp*)be_app;

	for (std::map<std::string, mtx::responses::InvitedRoom>::iterator iter
				= invites.begin();
			iter != invites.end();
			++iter)
	{
		const char* chat_id = iter->first.c_str();
		mtx::responses::InvitedRoom room = iter->second;

		BMessage inviteMsg(IM_MESSAGE);
		inviteMsg.AddInt32("im_what", IM_ROOM_INVITE_RECEIVED);
		inviteMsg.AddString("chat_id", chat_id);

		// Grab the inviter & name, if available
		for (const auto &e : room.invite_state) {
			auto invite = std::get_if<mtx::events::StrippedEvent<
				mtx::events::state::Member>>(&e);
			if (invite != nullptr
					&& invite->content.membership
						== mtx::events::state::Membership::Invite)
				inviteMsg.AddString("user_id", invite->sender.c_str());

			auto name = std::get_if<mtx::events::StrippedEvent<
				mtx::events::state::Name>>(&e);
			if (name != nullptr)
				inviteMsg.AddString("chat_name", name->content.name.c_str());
		}

		app->SendMessage(inviteMsg);
	}
}


void
room_sync(mtx::responses::Rooms rooms)
{
	MatrixApp* app = (MatrixApp*)be_app;

	std::map<std::string, mtx::responses::JoinedRoom> joined = rooms.join;
	for (std::map<std::string, mtx::responses::JoinedRoom>::iterator iter
				= joined.begin();
			iter != joined.end();
			++iter)
	{
		const char* chat_id = iter->first.c_str();
		mtx::responses::JoinedRoom room = iter->second;

		bool isNewRoom = app->fRoomList.HasString(BString(chat_id)) == false;
		if (isNewRoom) {
			BMessage joinedMsg(IM_MESSAGE);
			joinedMsg.AddInt32("im_what", IM_ROOM_JOINED);
			joinedMsg.AddString("chat_id", chat_id);
			((MatrixApp*)be_app)->SendMessage(joinedMsg);

			app->fRoomList.Add(BString(chat_id));
		}

		// Grab the latest room metadata
		BMessage metadataMsg(IM_MESSAGE);
		metadataMsg.AddInt32("im_what", IM_ROOM_METADATA);
		metadataMsg.AddString("chat_id", chat_id);
		metadataMsg.AddInt32("room_default_flags",
			(int32)(ROOM_LOG_LOCALLY | ROOM_POPULATE_LOGS | ROOM_NOTIFY_DM));
		metadataMsg.AddInt32("room_disallowed_flags", (int32)0);
		for (const auto &e : room.state.events) {
			auto name = std::get_if<mtx::events::StateEvent<mtx::events::state::Name>>(&e);
			if (name != nullptr && !name->content.name.empty())
				metadataMsg.AddString("chat_name", name->content.name.c_str());

			auto topic = std::get_if<mtx::events::StateEvent<mtx::events::state::Topic>>(&e);
			if (topic != nullptr && !topic->content.topic.empty())
				metadataMsg.AddString("subject", topic->content.topic.c_str());
		}
		((MatrixApp*)be_app)->SendMessage(metadataMsg);

		room_member_sync(chat_id, room, isNewRoom);

		// Grab the room timeline and add it
		for (mtx::events::collections::TimelineEvents &ev : room.timeline.events) {
			if (auto event = std::get_if<mtx::events::RoomEvent<mtx::events::msg::Text>>(&ev);
					event != nullptr)
			{
				BMessage msg(IM_MESSAGE);
				msg.AddInt32("im_what", IM_MESSAGE_RECEIVED);
				msg.AddString("body", event->content.body.c_str());
				msg.AddInt64("when", event->origin_server_ts / 1000);
				msg.AddString("chat_id", chat_id);
				msg.AddString("user_id", event->sender.c_str());
				app->SendMessage(msg);
			}
			else if (auto notice = std::get_if<mtx::events::RoomEvent<mtx::events::msg::Notice>>(&ev);
					notice != nullptr)
			{
				BMessage msg(IM_MESSAGE);
				msg.AddInt32("im_what", IM_MESSAGE_RECEIVED);
				msg.AddString("body", notice->content.body.c_str());
				msg.AddInt64("when", notice->origin_server_ts / 1000);
				msg.AddString("chat_id", chat_id);
				msg.AddString("user_id", notice->sender.c_str());
				app->SendMessage(msg);
			}
		}
	}

	std::map<std::string, mtx::responses::LeftRoom> left = rooms.leave;
	for (std::map<std::string, mtx::responses::LeftRoom>::iterator iter
				= left.begin();
			iter != left.end();
			++iter)
	{
		const char* chat_id = iter->first.c_str();
		if (app->fRoomList.HasString(BString(chat_id)) == true)
			app->fRoomList.Remove(BString(chat_id));

		BMessage leftMsg(IM_MESSAGE);
		leftMsg.AddInt32("im_what", IM_ROOM_LEFT);
		leftMsg.AddString("chat_id", ((std::string)iter->first).c_str());
		((MatrixApp*)be_app)->SendMessage(leftMsg);
	}
}


void
room_member_sync(const std::string& chat_id, mtx::responses::JoinedRoom& room,
	bool isNewRoom)
{
	MatrixApp* app = (MatrixApp*)be_app;

	// Newly-seen rooms get their memberlist quietly populated…
	if (isNewRoom) {
		BMessage members(IM_MESSAGE);
		members.AddInt32("im_what", IM_ROOM_PARTICIPANTS);
		members.AddString("chat_id", chat_id.c_str());
		for (const auto &e : room.state.events) {
			auto ev = std::get_if<mtx::events::StateEvent<mtx::events::state::Member>>(&e);
			if (ev == nullptr
					|| ev->content.membership != mtx::events::state::Membership::Join)
				continue;
			members.AddString("user_id", ev->state_key.c_str());
			if (!ev->content.display_name.empty())
				members.AddString("user_name", ev->content.display_name.c_str());
		}
		app->SendMessage(members);
		return;
	}

	// …while known rooms only get membership _changes_.
	// (These can arrive in either the state delta or the timeline.
	// Guard against duplicates via the event id.)
	BStringList seen;
	auto handleMember = [&](const auto &e) {
		auto ev = std::get_if<mtx::events::StateEvent<mtx::events::state::Member>>(&e);
		if (ev == nullptr)
			return;

		if (!ev->event_id.empty() && seen.HasString(BString(ev->event_id.c_str())))
			return;
		if (!ev->event_id.empty())
			seen.Add(BString(ev->event_id.c_str()));

		if (ev->content.membership == mtx::events::state::Membership::Join) {
			BMessage joined(IM_MESSAGE);
			joined.AddInt32("im_what", IM_ROOM_PARTICIPANT_JOINED);
			joined.AddString("chat_id", chat_id.c_str());
			joined.AddString("user_id", ev->state_key.c_str());
			if (!ev->content.display_name.empty())
				joined.AddString("user_name", ev->content.display_name.c_str());
			app->SendMessage(joined);
		}
		else if (ev->content.membership == mtx::events::state::Membership::Leave
				|| ev->content.membership == mtx::events::state::Membership::Ban) {
			BMessage left(IM_MESSAGE);
			left.AddInt32("im_what", IM_ROOM_PARTICIPANT_LEFT);
			left.AddString("chat_id", chat_id.c_str());
			left.AddString("user_id", ev->state_key.c_str());
			app->SendMessage(left);
		}
	};

	for (const auto &e : room.state.events)
		handleMember(e);
	for (const auto &e : room.timeline.events)
		handleMember(e);
}
