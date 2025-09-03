// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ACropActor.generated.h"

UCLASS()
class TESTES_API AACropActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AACropActor();

	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* MeshComponent;

	void SetCropMesh(UStaticMesh* Mesh);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
