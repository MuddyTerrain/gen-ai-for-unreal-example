// Copyright 2024, Prajwal Shetty. All rights reserved.

#include "GXChatManagerExample.h"
#include "Models/OpenAI/GenOAIChat.h"
#include "Utilities/GenUtils.h"
#include "Utilities/GenGlobalDefinitions.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "Engine/World.h" // Required for GetNetMode()

AGXChatManagerExample::AGXChatManagerExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGXChatManagerExample::BeginPlay()
{
	Super::BeginPlay();

	// Create and display the dedicated UI for the C++ example
	// CORRECTED: Replaced deprecated IsClient() with GetNetMode() check.
	// This ensures UI is not created on a dedicated server.
	if (GetWorld() && GetWorld()->GetNetMode() != NM_DedicatedServer && CppChatWidgetClass)
	{
		ChatWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), CppChatWidgetClass);
		if (ChatWidgetInstance)
		{
			// The widget will be responsible for getting a reference to this actor.
			// A common way is for the widget to "Get Actor of Class" on its BeginPlay.
			ChatWidgetInstance->AddToViewport();
		}
	}
}

void AGXChatManagerExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Cancel any in-flight request to prevent callbacks on an invalid object
	if (ActiveChatRequest.IsValid() && (ActiveChatRequest->GetStatus() == EHttpRequestStatus::Processing || ActiveChatRequest->GetStatus() == EHttpRequestStatus::NotStarted))
	{
		UE_LOG(LogGenAI, Log, TEXT("Cancelling in-flight OpenAI chat request for %s."), *GetName());
		ActiveChatRequest->CancelRequest();
	}
	ActiveChatRequest.Reset();
	
	Super::EndPlay(EndPlayReason);
}

void AGXChatManagerExample::SendChatMessageFromUI(const FString& Message)
{
	if (Message.IsEmpty())
	{
		return;
	}

	// Add the user's message to history and trigger the UI update event
	ConversationHistory.Add(FGenChatMessage(TEXT("user"), Message));
	OnNewMessageReceived(TEXT("User: "), Message);

	// Prepare the settings for the API call
	FGenOAIChatSettings ChatSettings;
	ChatSettings.Model = EOpenAIChatModel::GPT_4o_mini;
	ChatSettings.Messages = ConversationHistory;
	ChatSettings.MaxTokens = 1000;
	ChatSettings.bStream = false;

	// Use the static C++ function from your plugin
	ActiveChatRequest = UGenOAIChat::SendChatRequest(
		ChatSettings,
		FOnChatCompletionResponse::CreateLambda(
			[this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
			{
				// Ensure the actor hasn't been destroyed while the request was in flight
				if (!UGenUtils::IsContextStillValid(this)) return;

				if (bSuccess)
				{
					ConversationHistory.Add(FGenChatMessage(TEXT("assistant"), Response));
					OnNewMessageReceived(TEXT("AI: "), Response); // Trigger UI update
				}
				else
				{
					OnNewMessageReceived(TEXT("Error: "), ErrorMessage); // Trigger UI update
				}
				
				ActiveChatRequest.Reset();
			})
	);
}
