// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GXOpenAIStructuredOpExample.generated.h"

UCLASS()
class GENAIEXAMPLE_API AGXOpenAIStructuredOpExample : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGXOpenAIStructuredOpExample();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
