//$ Copyright 2015-19, Code Respawn Technologies Pvt Ltd - All Rights Reserved $//

#include "ConstructionSystem/ConstructionSystemCursor.h"
#include "PrefabTools.h"
#include "PrefabActor.h"
#include "Components/PrimitiveComponent.h"
#include "PrefabricatorAsset.h"
#include "PrefabComponent.h"
#include "PrefabricatorFunctionLibrary.h"
#include "ConstructionSystemSnap.h"

void UConstructionSystemCursor::RecreateCursor(UWorld* InWorld, UPrefabricatorAssetInterface* InCursorPrefab)
{
	DestroyCursor();

	if (InWorld) {
		if (InCursorPrefab) {
			CursorGhostActor = InWorld->SpawnActor<APrefabActor>();
			CursorGhostActor->PrefabComponent->PrefabAssetInterface = InCursorPrefab;


			FRandomStream RandomStream(CursorSeed);
			UPrefabricatorBlueprintLibrary::RandomizePrefab(CursorGhostActor, RandomStream);
			CursorGhostActor->GetRootComponent()->SetMobility(EComponentMobility::Movable);

			FPrefabTools::IterateChildrenRecursive(CursorGhostActor, [this](AActor* ChildActor) {
				for (UActorComponent* Component : ChildActor->GetComponents()) {
					if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component)) {
						if (!Primitive->IsA<UPrefabricatorConstructionSnapComponent>()) {
							// Disable collision
							Primitive->SetCollisionEnabled(ECollisionEnabled::NoCollision);
						}
					}

					if (UPrefabricatorConstructionSnapComponent* SnapComponent = Cast<UPrefabricatorConstructionSnapComponent>(Component)) {
						SnapComponents.Add(SnapComponent);
					}
				}
			});
		}
		else {
			// No active prefab. destroy the cursor actors
			if (CursorGhostActor) {
				CursorGhostActor->Destroy();
				CursorGhostActor = nullptr;
			}
		}
	}

	SetVisiblity(EConstructionSystemCursorVisiblity::Visible, true);
}

void UConstructionSystemCursor::AssignMaterialRecursive(UMaterialInterface* InMaterial) const
{
	if (CursorGhostActor && InMaterial) {
		FPrefabTools::IterateChildrenRecursive(CursorGhostActor, [this, InMaterial](AActor* ChildActor) {
			for (UActorComponent* Component : ChildActor->GetComponents()) {
				if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component)) {
					// Set cursor material
					int32 NumMaterials = Primitive->GetNumMaterials();
					for (int ElementIndex = 0; ElementIndex < NumMaterials; ElementIndex++) {
						Primitive->SetMaterial(ElementIndex, InMaterial);
					}
				}
			}
		});
	}
}

void UConstructionSystemCursor::DestroyCursor()
{
	if (CursorGhostActor) {
		SnapComponents.Reset();
		CursorGhostActor->Destroy();
		ActiveSnapComponentIndex = 0;
		CursorGhostActor = nullptr;
	}
}

void UConstructionSystemCursor::SetVisiblity(EConstructionSystemCursorVisiblity InVisiblity, bool bForce)
{
	if (!bForce && Visiblity == InVisiblity) {
		return;
	}
	Visiblity = InVisiblity;

	if (CursorGhostActor) {
		FPrefabTools::IterateChildrenRecursive(CursorGhostActor, [this, InVisiblity](AActor* ChildActor) {
			ChildActor->SetActorHiddenInGame(InVisiblity == EConstructionSystemCursorVisiblity::Hidden);
		});
		if (Visiblity != EConstructionSystemCursorVisiblity::Hidden) {
			// Update the material
			UMaterialInterface* Material = (Visiblity == EConstructionSystemCursorVisiblity::VisibleInvalid) ? CursorInvalidMaterial : CursorMaterial;
			AssignMaterialRecursive(Material);
		}
	}
}

void UConstructionSystemCursor::SetTransform(const FTransform& InTransform)
{
	if (CursorGhostActor) {
		CursorGhostActor->SetActorTransform(InTransform);
	}
}

bool UConstructionSystemCursor::GetCursorTransform(FTransform& OutTransform) const
{
	if (CursorGhostActor) {
		OutTransform = CursorGhostActor->GetActorTransform();
		return true;
	}
	return false;
}

void UConstructionSystemCursor::MoveToNextSnapComponent()
{
	if (SnapComponents.Num() > 0) {
		ActiveSnapComponentIndex = (ActiveSnapComponentIndex + 1) % SnapComponents.Num();
	}
}

void UConstructionSystemCursor::MoveToPrevSnapComponent()
{
	if (SnapComponents.Num() > 0) {
		ActiveSnapComponentIndex = (ActiveSnapComponentIndex - 1);
		if (ActiveSnapComponentIndex < 0) {
			ActiveSnapComponentIndex += SnapComponents.Num();
		}
	}
}

UPrefabricatorConstructionSnapComponent* UConstructionSystemCursor::GetActiveSnapComponent()
{
	if (ActiveSnapComponentIndex < 0 || ActiveSnapComponentIndex >= SnapComponents.Num()) {
		return nullptr;
	}
	return SnapComponents[ActiveSnapComponentIndex];
}
