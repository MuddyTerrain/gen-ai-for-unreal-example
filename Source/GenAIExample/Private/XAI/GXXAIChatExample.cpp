// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "XAI/GXXAIChatExample.h"
#include "Models/XAI/GenXAIChat.h"
#include "Models/XAI/GenXAIChatStream.h"
#include "Data/XAI/GenXAIChatStructs.h"
#include "Utilities/GenUtils.h"
#include "Misc/Paths.h"

AGXXAIChatExample::AGXXAIChatExample()
{
	PrimaryActorTick.bCanEverTick = false;
	// Add a default system message to guide the AI's behavior
	ConversationHistory.Add(FGenXAIMessage(TEXT("system"), {FGenAIMessageContent::FromText(TEXT("You are a helpful assistant integrated into an Unreal Engine application."))}));
}

void AGXXAIChatExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Cancel any in-flight requests to prevent crashes on actor destruction
	if (ActiveRequestNonStreaming.IsValid() && ActiveRequestNonStreaming->GetStatus() == EHttpRequestStatus::Processing)
	{
		ActiveRequestNonStreaming->CancelRequest();
	}
	if (ActiveRequestStreaming.IsValid() && ActiveRequestStreaming->GetStatus() == EHttpRequestStatus::Processing)
	{
		ActiveRequestStreaming->CancelRequest();
	}
	Super::EndPlay(EndPlayReason);
}

void AGXXAIChatExample::RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, UTexture2D* Image)
{
	if (ActiveRequestNonStreaming.IsValid()) return;

	// 1. Construct the multimodal message content
	TArray<FGenAIMessageContent> MessageContent;
	MessageContent.Add(FGenAIMessageContent::FromText(UserMessage));
	if (Image != nullptr)
	{
		// XAI API expects a base64-encoded image URL string
		MessageContent.Add(FGenAIMessageContent::FromTexture2D(Image, EGenAIImageDetail::Auto));
	}

	// 2. Add the complete user message to our history
	ConversationHistory.Add(FGenXAIMessage(TEXT("user"), MessageContent));

	// 3. Configure the chat settings
	FGenXAIChatSettings ChatSettings;
	// Set the model directly as string
	ChatSettings.Model = ModelName;
	ChatSettings.Messages = ConversationHistory;

	// 4. Send the request using a lambda for the callback
	ActiveRequestNonStreaming = UGenXAIChat::SendChatRequest(
		ChatSettings,
		FOnXAIChatCompletionResponse::CreateLambda(
			[this](const FString& Response, const FString& Error, bool bSuccess)
			{
				if (!UGenUtils::IsContextStillValid(this)) return;

				if (bSuccess)
				{
					// Add AI's response to history and broadcast to UI
					ConversationHistory.Add(FGenXAIMessage(TEXT("assistant"), {FGenAIMessageContent::FromText(Response)}));
					OnUINonStreamingResponse.Broadcast(Response, true);
				}
				else
				{
					// Broadcast error to UI and pop the failed user message
					OnUINonStreamingResponse.Broadcast(Error, false);
					ConversationHistory.Pop();
				}
				ActiveRequestNonStreaming.Reset();
			})
	);
}

void AGXXAIChatExample::RequestStreamingChat(const FString& UserMessage, const FString& ModelName, UTexture2D* Image)
{
	if (ActiveRequestStreaming.IsValid()) return;

	// 1. Construct the multimodal message content
	TArray<FGenAIMessageContent> MessageContent;
	MessageContent.Add(FGenAIMessageContent::FromText(UserMessage));
	if (Image != nullptr)
	{
		MessageContent.Add(FGenAIMessageContent::FromTexture2D(Image, EGenAIImageDetail::Auto));
	}

	// 2. Add to history
	ConversationHistory.Add(FGenXAIMessage(TEXT("user"), MessageContent));

	// 3. Configure settings
	FGenXAIChatSettings ChatSettings;
	// Set the model directly as string
	ChatSettings.Model = ModelName;
	ChatSettings.Messages = ConversationHistory;

	// 4. Send the request, binding our handler function to the delegate
	ActiveRequestStreaming = UGenXAIChatStream::SendStreamChatRequest(ChatSettings, FOnXAIChatStreamResponse::CreateUObject(this, &AGXXAIChatExample::OnStreamingChatEvent));
}

void AGXXAIChatExample::ClearConversation()
{
	ConversationHistory.Empty();
	// Re-add the initial system message after clearing
	ConversationHistory.Add(FGenXAIMessage(TEXT("system"), {FGenAIMessageContent::FromText(TEXT("You are a helpful assistant integrated into an Unreal Engine application."))}));
}

void AGXXAIChatExample::OnStreamingChatEvent(EXAIStreamEventType EventType, const FString& Payload, bool bSuccess)
{
	if (!UGenUtils::IsContextStillValid(this)) return;

	if (!bSuccess)
	{
		OnUIStreamingError.Broadcast(Payload);
		ConversationHistory.Pop();
		ActiveRequestStreaming.Reset();
		return;
	}

	switch (EventType)
	{
		case EXAIStreamEventType::ContentDelta:
			// The payload is a delta chunk of the message.
			OnUIStreamingResponseDelta.Broadcast(Payload);
			break;

		case EXAIStreamEventType::Completion:
			// The payload is the final, complete message.
			ConversationHistory.Add(FGenXAIMessage(TEXT("assistant"), {FGenAIMessageContent::FromText(Payload)}));
			OnUIStreamingResponseCompleted.Broadcast(Payload);
			ActiveRequestStreaming.Reset();
			break;

		case EXAIStreamEventType::Error:
			 // This case is now handled by the initial !bSuccess check, but we keep it for clarity.
			 OnUIStreamingError.Broadcast(Payload);
			 ConversationHistory.Pop();
			 ActiveRequestStreaming.Reset();
			 break;
	}
}

// Simple helper to use model name directly as string
static FString GetXAIModelFromString(const FString& ModelName)
{
    return ModelName;
}
