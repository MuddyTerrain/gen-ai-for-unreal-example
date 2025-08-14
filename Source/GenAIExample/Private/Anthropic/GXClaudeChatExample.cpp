// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "Anthropic/GXClaudeChatExample.h"
#include "Models/Anthropic/GenClaudeChat.h"
#include "Utilities/GenUtils.h"
#include "Misc/Paths.h"


AGXClaudeChatExample::AGXClaudeChatExample()
{
    PrimaryActorTick.bCanEverTick = false;
    // Set a default system message
    ConversationHistory.Add(FGenClaudeChatMessage(TEXT("system"), TEXT("You are Claude, a helpful AI assistant integrated into an Unreal Engine application.")));
}

void AGXClaudeChatExample::ClearConversation()
{
    ConversationHistory.Empty();
    ConversationHistory.Add(FGenClaudeChatMessage(TEXT("system"), TEXT("You are Claude, a helpful AI assistant integrated into an Unreal Engine application.")));
}

void AGXClaudeChatExample::RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, UTexture2D* Image)
{
    // 1. Construct the message content (text and optional image)
    TArray<FGenAIMessageContent> MessageContent;
    MessageContent.Add(FGenAIMessageContent::FromText(UserMessage));
    
    if (Image != nullptr)
    {
        MessageContent.Add(FGenAIMessageContent::FromTexture2D(Image, EGenAIImageDetail::Auto));
    }

    // 2. Add the user message to history
    ConversationHistory.Add(FGenClaudeChatMessage(TEXT("user"), MessageContent));

    // 3. Configure chat settings
    FGenClaudeChatSettings ChatSettings;
    ChatSettings.Model = ModelName;  // Set model directly as string
    ChatSettings.Messages = ConversationHistory;
    ChatSettings.MaxTokens = 1500;
    ChatSettings.bStreamResponse = false;

    // 4. Send the request
    ActiveRequestNonStreaming = UGenClaudeChat::SendChatRequest(
        ChatSettings,
        FOnClaudeChatCompletionResponse::CreateLambda(
            [this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
            {
                if (!UGenUtils::IsContextStillValid(this)) return;

                if (bSuccess)
                {
                    // Add AI's response to history and broadcast to UI
                    ConversationHistory.Add(FGenClaudeChatMessage(TEXT("assistant"), Response));
                    OnUINonStreamingResponse.Broadcast(Response, true);
                }
                else
                {
                    // Broadcast error to UI
                    OnUINonStreamingResponse.Broadcast(ErrorMessage, false);
                    // Remove the user message on failure
                    ConversationHistory.Pop();
                }
                ActiveRequestNonStreaming.Reset();
            })
    );
}

void AGXClaudeChatExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Cancel any active requests
    if (ActiveRequestNonStreaming.IsValid())
    {
        ActiveRequestNonStreaming->CancelRequest();
    }
    
    Super::EndPlay(EndPlayReason);
}
