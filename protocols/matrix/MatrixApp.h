/*
 * Copyright 2021, Jaidyn Levesque <jadedctrl@teknik.io>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _MATRIX_APP_H
#define _MATRIX_APP_H

#include <Application.h>
#include <String.h>
#include <StringList.h>

#include <map>
#include <string>

#include <mtx.hpp>
#include <mtxclient/http/client.hpp>
#include <mtxclient/http/errors.hpp>


const uint32 CHECK_APP = 'Paca';


class MatrixApp : public BApplication {
public:
						MatrixApp();

	virtual void		MessageReceived(BMessage* msg);
			void		ImMessage(BMessage* msg);

			void		Connect();
			void		Disconnect();
			void		StartLoop();

			void		SendMessage(BMessage msg);
			void		SendError(const char* message,
							mtx::http::RequestErr err, bool fatal = false);

			void		RoomSendMessage(const char* chat_id,
							const char* body);
			void		RoomJoin(const char* chat_id);
			void		RoomLeave(const char* chat_id);
			void		RoomGetParticipants(const char* chat_id);
			void		SetNickname(const char* display_name);

	BMessage* fSettings;
	status_t fInitStatus;
	BStringList fRoomList;

private:
	// Settings
	BString fUser;
	BString fPassword;
	BString fServer;
	BString fSession;

	thread_id fProtoThread;
	bool fConnected;
};


void print_error(mtx::http::RequestErr err, const char* message = NULL);

void initial_sync_handler(const mtx::responses::Sync &res, mtx::http::RequestErr err);
void sync_handler(const mtx::responses::Sync &res, mtx::http::RequestErr err);

void room_sync(mtx::responses::Rooms rooms);
void invite_sync(std::map<std::string, mtx::responses::InvitedRoom> invites);
void room_member_sync(const std::string& chat_id,
	mtx::responses::JoinedRoom& room, bool isNewRoom);

#endif // _MATRIX_APP_H
