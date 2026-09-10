/*
 * Copyright 2021, Jaidyn Levesque <jadedctrl@teknik.io>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "MatrixProtocol.h"

#include <Catalog.h>
#include <Messenger.h>
#include <OS.h>
#include <Roster.h>

#include <libinterface/BitmapUtils.h>

#include <ChatProtocolMessages.h>
#include <UserStatus.h>

#include "Matrix.h"
#include "MatrixMessages.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MatrixProtocol"


status_t
connect_thread(void* data)
{
	MatrixProtocol* protocol = (MatrixProtocol*)data;
	while (true) {
		BMessage msg = receive_message();
		if (msg.what == 0) {
			snooze(100000);
			continue;
		}

		switch (msg.what) {
			case MATRIX_ACCOUNT_REGISTERED:
			{
				team_id team = (team_id)msg.GetInt64("team_id", -1);
				protocol->RegisterApp(team);
				break;
			}
			default:
				protocol->SendMessage(new BMessage(msg));
		}
	}
	return B_OK;
}


BMessage
receive_message()
{
	thread_id sender;
	int32 size = receive_data(&sender, NULL, 0);
	if (size <= 0)
		return BMessage();

	char buffer[size];
	receive_data(&sender, buffer, size);

	BMessage temp;
	if (temp.Unflatten(buffer) != B_OK)
		return BMessage();
	return temp;
}


MatrixProtocol::MatrixProtocol()
	:
	fSettings(NULL),
	fAppTeam(-1),
	fRecvThread(-1),
	fAppMessenger(NULL)
{
}


MatrixProtocol::~MatrixProtocol()
{
	Shutdown();
	delete fSettings;
}


status_t
MatrixProtocol::Init(ChatProtocolMessengerInterface* interface)
{
	fMessenger = interface;
	return B_OK;
}


status_t
MatrixProtocol::Shutdown()
{
	_GoOffline();
	if (fRecvThread >= 0)
		kill_thread(fRecvThread);
	return B_OK;
}


status_t
MatrixProtocol::UpdateSettings(BMessage* settings)
{
	if (fRecvThread < B_OK) {
		fRecvThread = spawn_thread(connect_thread,
			"matrix connections", B_NORMAL_PRIORITY, (void*)this);
		if (fRecvThread < B_OK)
			return B_ERROR;
	}

	settings->AddInt64("thread_id", fRecvThread);
	delete fSettings;
	fSettings = new BMessage(*settings);
	return B_OK;
}


status_t
MatrixProtocol::Process(BMessage* msg)
{
	if (msg->what != IM_MESSAGE)
		return B_ERROR;

	int32 im_what = msg->GetInt32("im_what", -1);

	switch (im_what) {
		case IM_SET_OWN_STATUS:
		{
			int32 status = msg->GetInt32("status", -1);
			switch (status) {
				case STATUS_ONLINE:
					_GoOnline();
					break;
				case STATUS_OFFLINE:
					_GoOffline();
					break;
				default:
					_SendMatrixMessage(new BMessage(*msg));
			}
			break;
		}
		default:
			_SendMatrixMessage(new BMessage(*msg));
	}
	return B_OK;
}


BMessage
MatrixProtocol::SettingsTemplate(const char* name)
{
	BMessage settings;
	if (strcmp(name, "account") == 0)
		settings = _AccountTemplate();
	else if (strcmp(name, "join_room") == 0 || strcmp(name, "create_room") == 0)
		settings = _RoomTemplate();
	else if (strcmp(name, "roster") == 0)
		settings = _RosterTemplate();
	return settings;
}


BObjectList<BMessage>
MatrixProtocol::Commands()
{
	return BObjectList<BMessage>();
}


BBitmap*
MatrixProtocol::Icon() const
{
	return ReadNodeIcon(fAddOnPath.Path(), B_LARGE_ICON, true);
}


void
MatrixProtocol::SendMessage(BMessage* msg)
{
	msg->AddString("protocol", Signature());
	fMessenger->SendMessage(msg);
}


void
MatrixProtocol::RegisterApp(team_id team)
{
	if (team < 0)
		return;
	fAppTeam = team;
	delete fAppMessenger;
	fAppMessenger = new BMessenger(NULL, team);
}


void
MatrixProtocol::_SendMatrixMessage(BMessage* msg)
{
	msg->AddString("protocol", MATRIX_ADDON);
	if (fAppMessenger != NULL && fAppMessenger->IsValid())
		fAppMessenger->SendMessage(msg);
}


void
MatrixProtocol::_StartApp()
{
	BMessage* start = new BMessage(*fSettings);
	start->what = MATRIX_REGISTER_ACCOUNT;

	BRoster roster;
	if (roster.Launch(MATRIX_SIGNATURE, start) == B_OK)
		snooze(100000);
}


void
MatrixProtocol::_GoOnline()
{
	// Make sure the connection thread is alive, respawning it if
	// it was killed when going offline before.
	thread_info info;
	if (fRecvThread < B_OK || get_thread_info(fRecvThread, &info) != B_OK) {
		fRecvThread = spawn_thread(connect_thread, "matrix connections",
			B_NORMAL_PRIORITY, (void*)this);
	}
	if (fRecvThread >= B_OK)
		resume_thread(fRecvThread);

	if (fAppMessenger == NULL || fAppMessenger->IsValid() == false)
		_StartApp();
}


void
MatrixProtocol::_GoOffline()
{
	if (fAppMessenger != NULL && fAppMessenger->IsValid())
		fAppMessenger->SendMessage(new BMessage(B_QUIT_REQUESTED));

	delete fAppMessenger;
	fAppMessenger = NULL;
	fAppTeam = -1;
}


BMessage
MatrixProtocol::_AccountTemplate()
{
	BMessage settings;

	BMessage server;
	server.AddString("name", "server");
	server.AddString("description", B_TRANSLATE("Homeserver:"));
	server.AddString("default", "matrix.org");
	server.AddString("error", B_TRANSLATE("Please enter a valid server address."));
	server.AddInt32("type", B_STRING_TYPE);
	settings.AddMessage("setting", &server);

	BMessage user;
	user.AddString("name", "username");
	user.AddString("description", B_TRANSLATE("Username:"));
	user.AddString("error", B_TRANSLATE("You need a username in order to connect!"));
	user.AddInt32("type", B_STRING_TYPE);
	settings.AddMessage("setting", &user);

	BMessage password;
	password.AddString("name", "password"); password.AddString("description", B_TRANSLATE("Password:"));
	password.AddString("error", B_TRANSLATE("Without a password, how will love survive?"));
	password.AddInt32("type", B_STRING_TYPE);
	settings.AddMessage("setting", &password);

	BMessage session;
	session.AddString("name", "session");
	session.AddString("description", B_TRANSLATE("Session name:"));
	session.AddInt32("type", B_STRING_TYPE);
	session.AddString("default", "Chat-O-Matic [Haiku]");
	settings.AddMessage("setting", &session);

	return settings;
}


BMessage
MatrixProtocol::_RoomTemplate()
{
	BMessage settings;

	BMessage id;
	id.AddString("name", "chat_id");
	id.AddString("description", B_TRANSLATE("Channel:"));
	id.AddString("error", B_TRANSLATE("Please enter a channel― skipping it doesn't make sense!"));
	id.AddInt32("type", B_STRING_TYPE);
	settings.AddMessage("setting", &id);

	return settings;
}


BMessage
MatrixProtocol::_RosterTemplate()
{
	BMessage settings;

	BMessage nick;
	nick.AddString("name", "user_id");
	nick.AddString("description", B_TRANSLATE("User ID:"));
	nick.AddString("error", B_TRANSLATE("How can someone be your friend if you don't know their ID?"));
	nick.AddInt32("type", B_STRING_TYPE);
	settings.AddMessage("setting", &nick);

	return settings;
}
