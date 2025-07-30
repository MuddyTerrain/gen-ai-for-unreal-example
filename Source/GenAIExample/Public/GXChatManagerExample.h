// Copyright 2024, Prajwal Shetty. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IHttpRequest.h"
#include "Data/OpenAI/GenOAIChatStructs.h"
#include "GXChatManagerExample.generated.h"

class UUserWidget;

/**
 * C++ Example Actor that demonstrates calling the OpenAI Chat API.
 * This actor creates and manages a dedicated UMG widget for its UI.
 */
UCLASS()
class GENAIEXAMPLE_API AGXChatManagerExample : public AActor
{
	GENERATED_BODY()
	
public:	
	AGXChatManagerExample();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** The dedicated Widget Blueprint to use for the C++ example's UI. */
	UPROPERTY(EditAnywhere, Category = "C++ Example")
	TSubclassOf<UUserWidget> CppChatWidgetClass;

private:
	/** A pointer to the instance of our UI widget. */
	UPROPERTY()
	TObjectPtr<UUserWidget> ChatWidgetInstance;

	/** A handle to the active HTTP request, used for cancellation on EndPlay. */
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveChatRequest;

	/** Stores the entire conversation history. */
	TArray<FGenChatMessage> ConversationHistory;

public:
	/**
	 * Public function for the dedicated C++ widget to call when the user sends a message.
	 */
	UFUNCTION(BlueprintCallable, Category = "C++ Example")
	void SendChatMessageFromUI(const FString& Message);

protected:
	/**
	 * A Blueprint-implementable event that this C++ class calls to update its UI.
	 * We will implement this event in the WBP_CppChat widget's graph.
	 * @param Sender The prefix (e.g., "User: " or "AI: ").
	 * @param Message The text to display.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "C++ Example")
	void OnNewMessageReceived(const FString& Sender, const FString& Message);
};
