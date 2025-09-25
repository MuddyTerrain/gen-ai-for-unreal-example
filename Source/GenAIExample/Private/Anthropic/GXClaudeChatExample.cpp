// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "Anthropic/GXClaudeChatExample.h"

#if WITH_GENAI_MODULE
#include "Models/Anthropic/GenClaudeChat.h"
#include "Utilities/GenUtils.h"
#include "Misc/Paths.h"
#endif

AGXClaudeChatExample::AGXClaudeChatExample()
{
#if WITH_GENAI_MODULE
    PrimaryActorTick.bCanEverTick = false;
    // Set a default system message
    ConversationHistory.Add(FGenClaudeChatMessage(TEXT("system"), TEXT("You are Claude, a helpful AI assistant integrated into an Unreal Engine application.")));
#endif
}

void AGXClaudeChatExample::ClearConversation()
{
#if WITH_GENAI_MODULE
    ConversationHistory.Empty();
    ConversationHistory.Add(FGenClaudeChatMessage(TEXT("system"), TEXT("You are Claude, a helpful AI assistant integrated into an Unreal Engine application.")));
#else
    // Dummy implementation
#endif
}

void AGXClaudeChatExample::RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& SystemPrompt, UTexture2D* Image)
{
#if WITH_GENAI_MODULE
    if (!SystemPrompt.IsEmpty())
    {
        if (ConversationHistory.Num() > 0 && ConversationHistory[0].Role == TEXT("system"))
        {
            ConversationHistory[0] = FGenClaudeChatMessage(TEXT("system"), SystemPrompt);
        }
        else
        {
            ConversationHistory.Insert(FGenClaudeChatMessage(TEXT("system"), SystemPrompt), 0);
        }
    }
    
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
#else
    // Dummy implementation
    UE_LOG(LogTemp, Warning, TEXT("GenAI module is not available. RequestNonStreamingChat will do nothing."));
#endif
}

void AGXClaudeChatExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if WITH_GENAI_MODULE
    // Cancel any active requests
    if (ActiveRequestNonStreaming.IsValid())
    {
        ActiveRequestNonStreaming->CancelRequest();
    }
#endif
    Super::EndPlay(EndPlayReason);
}
