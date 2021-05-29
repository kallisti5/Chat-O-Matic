/*
 * Copyright 2009-2011, Andrea Anzani. All rights reserved.
 * Copyright 2009-2011, Pier Luigi Fiorini. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrea Anzani, andrea.anzani@gmail.com
 *		Pier Luigi Fiorini, pierluigi.fiorini@gmail.com
 */

#include <Application.h>
#include <Alert.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Notification.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>
#include <TranslationUtils.h>

#include <libinterface/BitmapUtils.h>

#include "AccountManager.h"
#include "CayaConstants.h"
#include "CayaMessages.h"
#include "CayaProtocolMessages.h"
#include "CayaPreferences.h"
#include "ConversationItem.h"
#include "ConversationListView.h"
#include "ConversationView.h"
#include "EditingFilter.h"
#include "NotifyMessage.h"
#include "MainWindow.h"
#include "PreferencesDialog.h"
#include "ReplicantStatusView.h"
#include "RosterWindow.h"
#include "Server.h"
#include "StatusView.h"


const uint32 kLogin			= 'LOGI';


MainWindow::MainWindow()
	:
	BWindow(BRect(0, 0, 300, 400), "Caya", B_TITLED_WINDOW, 0),
	fWorkspaceChanged(false)
{
	fStatusView = new StatusView("statusView");

	// Menubar
	BMenuBar* menuBar = new BMenuBar("MenuBar");

	BMenu* programMenu = new BMenu("Program");
	programMenu->AddItem(new BMenuItem("About" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	programMenu->AddItem(new BMenuItem("Preferences" B_UTF8_ELLIPSIS,
		new BMessage(CAYA_SHOW_SETTINGS), ',', B_COMMAND_KEY));
	programMenu->AddItem(new BSeparatorItem());
	programMenu->AddItem(new BMenuItem("Quit",
		new BMessage(B_QUIT_REQUESTED), 'Q', B_COMMAND_KEY));
	programMenu->SetTargetForItems(this);

	BMenu* chatMenu = new BMenu("Chat");
	chatMenu->AddItem(new BMenuItem("New chat" B_UTF8_ELLIPSIS,
		new BMessage(CAYA_NEW_CHAT)));
	chatMenu->SetTargetForItems(this);

	menuBar->AddItem(programMenu);
	menuBar->AddItem(chatMenu);

	fListView = new ConversationListView("roomList");

	fChatView = new ConversationView();

	fSendView = new BTextView("fSendView");
	fSendScroll = new BScrollView("fSendScroll", fSendView,
		B_WILL_DRAW, false, true);
	fSendView->SetWordWrap(true);
	AddCommonFilter(new EditingFilter(fSendView));
	fSendView->MakeFocus(true);

	fRightView = new BGroupView("rightView", B_VERTICAL);
	fRightView->AddChild(fChatView);
	fRightView->AddChild(fSendScroll);


	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(menuBar)

		.AddGroup(B_HORIZONTAL)
			.SetInsets(5, 5, 5, 10)
			.AddGroup(B_VERTICAL)
				.Add(fListView)
				.Add(fStatusView)
			.End()

			.Add(fRightView)
		.End()
	.End();

	// Filter messages using Server
	fServer = new Server();
	AddFilter(fServer);

	CenterOnScreen();

	//TODO check for errors here
	ReplicantStatusView::InstallReplicant();
}


void
MainWindow::Start()
{
	// Login all accounts
	fServer->LoginAll();
}


bool
MainWindow::QuitRequested()
{
	int32 button_index = 0;
	if(!CayaPreferences::Item()->DisableQuitConfirm)
	{
		BAlert* alert = new BAlert("Closing", "Are you sure you want to quit?",
			"Yes", "No", NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING,
			B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		button_index = alert->Go();
	}

	if(button_index == 0) {
		fServer->Quit();
		CayaPreferences::Get()->Save();
		ReplicantStatusView::RemoveReplicant();
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	} else {
		return false;
	}
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case CAYA_SHOW_SETTINGS:
		{
			PreferencesDialog* dialog = new PreferencesDialog();
			dialog->Show();
			break;
		}

		case CAYA_NEW_CHAT:
		{
			RosterWindow* roster = new RosterWindow("Invite contact to chat"
			B_UTF8_ELLIPSIS, IM_CREATE_CHAT, new BMessenger(this), fServer);
			roster->Show();
			break;
		}

		case CAYA_REPLICANT_STATUS_SET:
		{
			int32 status;
			message->FindInt32("status", &status);
			AccountManager* accountManager = AccountManager::Get();
			accountManager->SetStatus((CayaStatus)status);
			break;
		}

		case CAYA_REPLICANT_SHOW_WINDOW:
		{
			if (LockLooper()) {
				SetWorkspaces(B_CURRENT_WORKSPACE);
				
				if ((IsMinimized() || IsHidden()) 
					|| fWorkspaceChanged) {
					Minimize(false);
					Show();
					fWorkspaceChanged = false;
				} else if ((!IsMinimized() || !IsHidden())
					|| (!fWorkspaceChanged)) {
					Minimize(true);
				}
				UnlockLooper();
			}
			break;
		}

		case CAYA_CHAT:
		{
			message->AddString("body", fSendView->Text());
			fChatView->MessageReceived(message);
			fSendView->SetText("");

			break;
		}

		case IM_MESSAGE:
			ImMessage(message);
			break;
		case IM_ERROR:
			ImError(message);
			break;
		case B_ABOUT_REQUESTED:
			be_app->PostMessage(message);
			break;

		default:
			BWindow::MessageReceived(message);
	}
}


void
MainWindow::ImError(BMessage* msg)
{
	const char* error = NULL;
	const char* detail = msg->FindString("detail");

	if (msg->FindString("error", &error) != B_OK)
		return;

	// Format error message
	BString errMsg(error);
	if (detail)
		errMsg << "\n" << detail;

	BAlert* alert = new BAlert("Error", errMsg.String(), "OK", NULL, NULL,
		B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go();
}


void
MainWindow::ImMessage(BMessage* msg)
{
	int32 im_what = msg->FindInt32("im_what");
	switch (im_what) {
		case IM_OWN_CONTACT_INFO:
			fStatusView->SetName(msg->FindString("name"));
			break;
		case IM_OWN_AVATAR_SET:
		{
			entry_ref ref;

			if (msg->FindRef("ref", &ref) == B_OK) {
				BBitmap* bitmap = BTranslationUtils::GetBitmap(&ref);
				fStatusView->SetAvatarIcon(bitmap);
			}
			break;
		}
		case IM_MESSAGE_RECEIVED:
		{
			_EnsureConversationItem(msg);
			break;
		}
		case IM_MESSAGE_SENT:
		case IM_CHAT_CREATED:
		{
			_EnsureConversationItem(msg);
			break;
		}
	}
}


void
MainWindow::SetConversation(Conversation* chat)
{
	BView* current = fRightView->FindView("chatView");
	fRightView->RemoveChild(fRightView->FindView("chatView"));
	fRightView->RemoveChild(fRightView->FindView("fSendScroll"));

	fChatView = chat->GetView();

	fRightView->AddChild(fChatView);
	fRightView->AddChild(fSendScroll);
}


void
MainWindow::ObserveInteger(int32 what, int32 val)
{
	switch (what) {
		case INT_ACCOUNT_STATUS:
			fStatusView->SetStatus((CayaStatus)val);
			break;
	}
}


void
MainWindow::UpdateListItem(ConversationItem* item)
{
	if (fListView->HasItem(item) == true)
		fListView->InvalidateItem(fListView->IndexOf(item));
	else
		fListView->AddItem(item);
}


void
MainWindow::WorkspaceActivated(int32 workspace, bool active)
{
	if (active)
		fWorkspaceChanged = false;
	else
		fWorkspaceChanged = true;
}


ConversationItem*
MainWindow::_EnsureConversationItem(BMessage* msg)
{
	ChatMap chats = fServer->Conversations();

	BString chat_id = msg->FindString("chat_id");
	Conversation* chat = fServer->ConversationById(chat_id);

	if (chat != NULL) {
		ConversationItem* item = chat->GetConversationItem();
		if (fListView->HasItem(item)) {
			UpdateListItem(item);
		}
		else if (item != NULL) {
			fListView->AddItem(item);
		}
		return item;
	}

	return NULL;
}


