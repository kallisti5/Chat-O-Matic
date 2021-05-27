/*
 * Copyright 2009-2011, Andrea Anzani. All rights reserved.
 * Copyright 2009-2011, Pier Luigi Fiorini. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H
 
#include <Window.h>

#include "Observer.h"

class BCardLayout;
class BTextControl;

class ConversationItem;
class ConversationListView;
class Server;
class StatusView;
class RosterItem;


class MainWindow: public BWindow, public Observer {
public:
						MainWindow();

			void		Start();

	virtual	void		MessageReceived(BMessage* message);
			void		ImMessage(BMessage* msg);
			void		ImError(BMessage* msg);
	virtual	bool		QuitRequested();

	virtual void		WorkspaceActivated(int32 workspace,
							bool active);

			void		ObserveInteger(int32 what, int32 val);

			Server*		GetServer() const { return fServer; }

			void		UpdateListItem(ConversationItem* item);	

			int32		CountItems() const;
			RosterItem*	ItemAt(int index);

private:
			ConversationItem*		_EnsureConversationItem(BMessage* msg);

	ConversationListView* fListView;
	StatusView*			fStatusView;
	Server*				fServer;
	bool				fWorkspaceChanged;
};

#endif	// _MAIN_WINDOW_H
