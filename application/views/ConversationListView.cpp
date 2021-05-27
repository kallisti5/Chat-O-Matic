/*
 * Copyright 2021, Jaidyn Levesque <jadedctrl@teknik.io>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ConversationListView.h"

#include <PopUpMenu.h>
#include <MenuItem.h>
#include <Window.h>

#include "CayaMessages.h"
#include "Conversation.h"
#include "ConversationItem.h"


const uint32 kOpenSelectedChat = 'CVos';
const uint32 kLeaveSelectedChat = 'CVcs';


ConversationListView::ConversationListView(const char* name)
	: BOutlineListView(name)
{
}


void
ConversationListView::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case kOpenSelectedChat:
		{
			ConversationItem* item;
			int32 selIndex = CurrentSelection();

			if ((item = (ConversationItem*)ItemAt(selIndex)) != NULL)
				item->GetConversation()->ShowWindow(false, true);
			break;
		}

		default:
			BOutlineListView::MessageReceived(msg);
	}
}


void
ConversationListView::MouseDown(BPoint where)
{
	BListView::MouseDown(where);

	uint32 buttons = 0;
	Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);

	if (buttons & B_PRIMARY_MOUSE_BUTTON)
		MessageReceived(new BMessage(kOpenSelectedChat));

	if (!(buttons & B_SECONDARY_MOUSE_BUTTON))
		return;

	if (CurrentSelection() >= 0)
		_ConversationPopUp()->Go(ConvertToScreen(where), true, false);
	else
		_BlankPopUp()->Go(ConvertToScreen(where), true, false);
}


BPopUpMenu*
ConversationListView::_ConversationPopUp()
{
	BPopUpMenu* menu = new BPopUpMenu("chatPopUp");
	menu->AddItem(new BMenuItem("Open chat…", new BMessage(kOpenSelectedChat)));
	menu->AddItem(new BMenuItem("Leave chat", new BMessage(kLeaveSelectedChat)));
	menu->SetTargetForItems(this);

	return menu;

}


BPopUpMenu*
ConversationListView::_BlankPopUp()
{
	BPopUpMenu* menu = new BPopUpMenu("blankPopUp");
	menu->AddItem(new BMenuItem("New chat" B_UTF8_ELLIPSIS,
		new BMessage(CAYA_NEW_CHAT)));
	menu->SetTargetForItems(Window());
	
	return menu;
}


