// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "OpenAI/GXOpenAIChatExample.h"

#if WITH_GENAI_MODULE
#include "Models/OpenAI/GenOAIChat.h"
#include "Models/OpenAI/GenOAIChatStream.h"
#include "Utilities/GenUtils.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/EnumProperty.h"
#endif

#if WITH_GENAI_MODULE
// Simple helper function to get model name from settings
static FString GetModelFromSettings(const FGenOpenAIChatSettings& Settings)
{
	return Settings.Model;
}
#endif

AGXOpenAIChatExample::AGXOpenAIChatExample()
{
#if WITH_GENAI_MODULE
    PrimaryActorTick.bCanEverTick = false;
    // Set a default system message to guide the AI's behavior
    ConversationHistory.Add(FGenChatMessage(TEXT("system"), TEXT("You are a helpful assistant integrated into an Unreal Engine application.")));
#endif
}

void AGXOpenAIChatExample::ClearConversation()
{
#if WITH_GENAI_MODULE
    ConversationHistory.Empty();
    ConversationHistory.Add(FGenChatMessage(TEXT("system"), TEXT("You are a helpful assistant integrated into an Unreal Engine application.")));
#else
    // Dummy implementation
#endif
}

void AGXOpenAIChatExample::RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& SystemPrompt, UTexture2D* Image)
{
#if WITH_GENAI_MODULE
    if (!SystemPrompt.IsEmpty())
    {
        if (ConversationHistory.Num() > 0 && ConversationHistory[0].Role == TEXT("system"))
        {
            ConversationHistory[0] = FGenChatMessage(TEXT("system"), SystemPrompt);
        }
        else
        {
            ConversationHistory.Insert(FGenChatMessage(TEXT("system"), SystemPrompt), 0);
        }
    }

    // 1. Construct the message content (text and optional image)
    TArray<FGenAIMessageContent> MessageContent;
    MessageContent.Add(FGenAIMessageContent::FromText(UserMessage));
    
    if (Image != nullptr)
    {
        MessageContent.Add(FGenAIMessageContent::FromTexture2D(Image, EGenAIImageDetail::Auto));
    }

    // 2. Add the complete user message to our history
    ConversationHistory.Add(FGenChatMessage(TEXT("user"), MessageContent));

    // 3. Configure the chat settings
    FGenOpenAIChatSettings ChatSettings;
    
    // Set the model directly as string
    ChatSettings.Model = ModelName;
    ChatSettings.Messages = ConversationHistory;
    ChatSettings.MaxTokens = 1500;
    ChatSettings.bStream = false;

    // 4. Send the request using the static function and a lambda for the response
    ActiveRequestNonStreaming = UGenOAIChat::SendChatRequest(
        ChatSettings,
        FOnChatCompletionResponse::CreateLambda(
            [this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
            {
                if (!UGenUtils::IsContextStillValid(this)) return;
                
                if (bSuccess)
                {
                    // Add AI's response to history and broadcast to UI
                    ConversationHistory.Add(FGenChatMessage(TEXT("assistant"), Response));
                    OnUINonStreamingResponse.Broadcast(Response, true);
                }
                else
                {
                    // Broadcast error to UI
                    OnUINonStreamingResponse.Broadcast(ErrorMessage, false);
                    // Important: If the call failed, remove the user message we added
                    // so we don't have a one-sided conversation in our history.
                    ConversationHistory.Pop();
                }
                ActiveRequestNonStreaming.Reset();
            })
    );
#else
    UE_LOG(LogTemp, Warning, TEXT("GenAI module is not available. RequestNonStreamingChat will do nothing."));
#endif
}

void AGXOpenAIChatExample::RequestStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& SystemPrompt, UTexture2D* Image)
{
#if WITH_GENAI_MODULE
    if (!SystemPrompt.IsEmpty())
    {
        if (ConversationHistory.Num() > 0 && ConversationHistory[0].Role == TEXT("system"))
        {
            ConversationHistory[0] = FGenChatMessage(TEXT("system"), SystemPrompt);
        }
        else
        {
            ConversationHistory.Insert(FGenChatMessage(TEXT("system"), SystemPrompt), 0);
        }
    }

    // 1. Construct the message content
    TArray<FGenAIMessageContent> MessageContent;
    MessageContent.Add(FGenAIMessageContent::FromText(UserMessage));

    if (Image != nullptr)
    {
        MessageContent.Add(FGenAIMessageContent::FromTexture2D(Image, EGenAIImageDetail::Auto));
    }
    
    // 2. Add to history
    ConversationHistory.Add(FGenChatMessage(TEXT("user"), MessageContent));

    // 3. Configure settings
    FGenOpenAIChatSettings ChatSettings;
    
    // Set the model directly as string
    ChatSettings.Model = ModelName;
    ChatSettings.Messages = ConversationHistory;
    ChatSettings.bStream = true; // Implicitly handled, but good for clarity

    // 4. Send the request, binding our handler function to the delegate
    ActiveRequestStreaming = UGenOAIChatStream::SendStreamChatRequest(
        ChatSettings, 
        FOnOpenAIChatStreamResponse::CreateUObject(this, &AGXOpenAIChatExample::OnStreamingChatEvent)
    );
#else
    UE_LOG(LogTemp, Warning, TEXT("GenAI module is not available. RequestStreamingChat will do nothing."));
#endif
}

#if WITH_GENAI_MODULE
void AGXOpenAIChatExample::OnStreamingChatEvent(const FGenOpenAIStreamEvent& StreamEvent)
{
    if (!UGenUtils::IsContextStillValid(this)) return;

    if (!StreamEvent.bSuccess)
    {
        // Broadcast a specific error event for the UI
        OnUIStreamingError.Broadcast(StreamEvent.ErrorMessage);
        // Clean up on failure
        ConversationHistory.Pop(); 
        ActiveRequestStreaming.Reset();
        return;
    }

    switch (StreamEvent.EventType)
    {
        case EOpenAIStreamEventType::ResponseOutputTextDelta:
            // This is a chunk of text. Broadcast it to the UI.
            if (!StreamEvent.DeltaContent.IsEmpty())
            {
                OnUIStreamingResponseDelta.Broadcast(StreamEvent.DeltaContent);
            }
            break;

        case EOpenAIStreamEventType::ResponseCompleted:
            // The stream is done. The 'DeltaContent' now holds the rest of the message if any... 
            ConversationHistory.Add(FGenChatMessage(TEXT("assistant"), StreamEvent.DeltaContent));
            OnUIStreamingResponseCompleted.Broadcast(StreamEvent.DeltaContent);
            ActiveRequestStreaming.Reset();
            break;

        case EOpenAIStreamEventType::ResponseFailed:
        case EOpenAIStreamEventType::Error:
             OnUIStreamingError.Broadcast(StreamEvent.ErrorMessage);
             ConversationHistory.Pop();
             ActiveRequestStreaming.Reset();
             break;
        
        // Other cases like ResponseCreated can be ignored for UI purposes
        default:
            break;
    }
}
#endif

void AGXOpenAIChatExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if WITH_GENAI_MODULE
    // Cancel any in-flight requests when the actor is destroyed to prevent crashes.
    if (ActiveRequestNonStreaming.IsValid() && ActiveRequestNonStreaming->GetStatus() == EHttpRequestStatus::Processing)
    {
        ActiveRequestNonStreaming->CancelRequest();
    }
    if (ActiveRequestStreaming.IsValid() && ActiveRequestStreaming->GetStatus() == EHttpRequestStatus::Processing)
    {
        ActiveRequestStreaming->CancelRequest();
    }
#endif
    Super::EndPlay(EndPlayReason);
}