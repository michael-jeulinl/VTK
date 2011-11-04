/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3DButtonRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAssemblyPath.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkInteractorObserver.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkProp3D.h"
#include "vtkProp3DButtonRepresentation.h"
#include "vtkProp3DFollower.h"
#include "vtkPropPicker.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"
#include <vtkstd/map>

vtkStandardNewMacro(vtkProp3DButtonRepresentation);

struct vtkScaledProp
{
  vtkSmartPointer<vtkProp3D> Prop;
  vtkSmartPointer<vtkTransform> Transform;
  vtkScaledProp(){Transform = vtkTransform::New();}
};

// Map of textures
class vtkPropArray : public vtkstd::map<int,vtkScaledProp> {};
typedef vtkstd::map<int,vtkScaledProp>::iterator vtkPropArrayIterator;


//----------------------------------------------------------------------
vtkProp3DButtonRepresentation::vtkProp3DButtonRepresentation()
{
  // Current button representation
  this->CurrentProp = NULL;

  // Following
  this->FollowCamera = 0;
  this->Follower = vtkProp3DFollower::New();

  // List of textures
  this->PropArray = new vtkPropArray;

  this->Picker = vtkPropPicker::New();
  this->Picker->PickFromListOn();
}

//----------------------------------------------------------------------
vtkProp3DButtonRepresentation::~vtkProp3DButtonRepresentation()
{
  this->Follower->Delete();

  delete this->PropArray;

  this->Picker->Delete();
}

//-------------------------------------------------------------------------
void vtkProp3DButtonRepresentation::SetState(int state)
{
  this->Superclass::SetState(state);

  this->CurrentProp = this->GetButtonProp(this->State);
  this->Follower->SetProp3D(this->CurrentProp);

  this->Picker->InitializePickList();
  if ( this->CurrentProp )
    {
    this->Picker->AddPickList(this->CurrentProp);
    }
}

//-------------------------------------------------------------------------
void vtkProp3DButtonRepresentation::
SetButtonProp(int i, vtkProp3D *prop)
{
  if ( i < 0 )
    {
    i = 0;
    }
  if ( i >= this->NumberOfStates )
    {
    i = this->NumberOfStates - 1;
    }

  vtkScaledProp sprop;
  sprop.Prop = prop;

  (*this->PropArray)[i] = sprop;
}


//-------------------------------------------------------------------------
vtkProp3D *vtkProp3DButtonRepresentation::
GetButtonProp(int i)
{
  if ( i < 0 )
    {
    i = 0;
    }
  if ( i >= this->NumberOfStates )
    {
    i = this->NumberOfStates - 1;
    }

  vtkPropArrayIterator iter = this->PropArray->find(i);
  if ( iter != this->PropArray->end() )
    {
    return (*iter).second.Prop;
    }
  else
    {
    return NULL;
    }
}

//-------------------------------------------------------------------------
void vtkProp3DButtonRepresentation::PlaceWidget(double bds[6])
{
  double bounds[6], center[3], aBds[6];
  this->AdjustBounds(bds, bounds, center);

  if (this->InitialBounds[0] == bounds[0] && this->InitialBounds[1] == bounds[1] &&
      this->InitialBounds[2] == bounds[2] && this->InitialBounds[3] == bounds[3] &&
      this->InitialBounds[4] == bounds[4] && this->InitialBounds[5] == bounds[5])
    {
    return;
    }

  for (int i=0; i<6; i++)
    {
    this->InitialBounds[i] = bounds[i];
    }
  this->InitialLength = sqrt((bounds[1]-bounds[0])*(bounds[1]-bounds[0]) +
                             (bounds[3]-bounds[2])*(bounds[3]-bounds[2]) +
                             (bounds[5]-bounds[4])*(bounds[5]-bounds[4]));

  this->SetState(this->State);

  vtkProp3D *prop;
  vtkPropArrayIterator iter;
  for ( iter=this->PropArray->begin(); iter != this->PropArray->end(); ++iter )
    {
    prop = (*iter).second.Prop;
    prop->SetUserTransform(0);
    prop->GetBounds(aBds);

    if (!vtkMath::AreBoundsInitialized(aBds))
      {
      continue;
      }

    // Now fit the actor bounds in the place bounds by tampering with its
    // transform if one. In order to do that properly, we have to insert
    // the rescale and the translation in the proper place of the computationMatrix.
    // We apply the transformation in order to get the following pipeline:
    // [-Origin]*[Scale]*[Rescale]*[Rotation]*[Origin]*[Position]*[Translation].

    double origin[3], position[3], orientation[3];
    prop->GetOrigin(origin);
    prop->GetPosition(position);
    prop->GetOrientation(orientation);

    // Compute the center of the current bounds
    double aCenter[3];
    aCenter[0] = (aBds[0]+aBds[1]) / 2.0;
    aCenter[1] = (aBds[2]+aBds[3]) / 2.0;
    aCenter[2] = (aBds[4]+aBds[5]) / 2.0;

    // Calcul of the scale ratio
    double scale[3];
    for (int i=0; i < 3; ++i)
      {
      scale[i] = (bounds[2*i+1]-bounds[2*i]) / (aBds[2*i+1]-aBds[2*i]);
      }

    // Calcul of the translation
    double translation[3];
    translation[0] = center[0] - (aBds[0]+aBds[1]) / 2.0;
    translation[1] = center[1] - (aBds[2]+aBds[3]) / 2.0;
    translation[2] = center[2] - (aBds[4]+aBds[5]) / 2.0;

    vtkNew<vtkTransform> transform;
    transform->PostMultiply();

    // Inverse move back from origin and translate
    transform->Translate(-(origin[0] + position[0]) ,
                         -(origin[1] + position[1]),
                         -(origin[2] + position[2]));

    // Inverse rotation
    transform->RotateZ(-orientation[2]);
    transform->RotateX(-orientation[0]);
    transform->RotateY(-orientation[1]);

    // Apply our scale
    transform->Scale(scale);

    // Reapply the rotation
    transform->RotateY(orientation[1]);
    transform->RotateX(orientation[0]);
    transform->RotateZ(orientation[2]);

    // ReApply the move back from origin + translation
    transform->Translate(origin[0] + position[0],
                         origin[1] + position[1],
                         origin[2] + position[2]);

    // Apply our current translation
    transform->Translate(translation);

    // Keep the transform to fit the actor bounds
    (*iter).second.Transform = transform.GetPointer();
    }

  this->Modified();
  this->BuildRepresentation();
}

//-------------------------------------------------------------------------
int vtkProp3DButtonRepresentation
::ComputeInteractionState(int X, int Y, int vtkNotUsed(modify))
{
  this->VisibilityOn(); //actor must be on to be picked
  this->Picker->Pick(X,Y,0.0,this->Renderer);
  vtkAssemblyPath *path = this->Picker->GetPath();

  if ( path != NULL )
    {
    this->InteractionState = vtkButtonRepresentation::Inside;
    }
  else
    {
    this->InteractionState = vtkButtonRepresentation::Outside;
    }

  return this->InteractionState;
}

//----------------------------------------------------------------------
void vtkProp3DButtonRepresentation::BuildRepresentation()
{
  // The net effect is to resize the handle
  if ( this->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    this->SetState(this->State); //side effect sets CurrentProp
    vtkPropArrayIterator iter = this->PropArray->find(this->State);
    if ( this->CurrentProp == NULL || iter == this->PropArray->end() )
      {
      return;
      }

    // In case follower is being used
    if ( this->FollowCamera )
      {
      this->Follower->SetCamera(this->Renderer->GetActiveCamera());
      this->Follower->SetProp3D(this->CurrentProp);
      this->Follower->SetUserTransform((*iter).second.Transform);
      }
    else
      {
      this->CurrentProp->SetUserTransform((*iter).second.Transform);
      }

    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------
void vtkProp3DButtonRepresentation::ShallowCopy(vtkProp *prop)
{
  vtkProp3DButtonRepresentation *rep =
    vtkProp3DButtonRepresentation::SafeDownCast(prop);
  if ( rep )
    {
    vtkPropArrayIterator iter;
    for ( iter=rep->PropArray->begin();
          iter != rep->PropArray->end(); ++iter )
      {
      (*this->PropArray)[(*iter).first] = (*iter).second;
      }
    }
  this->FollowCamera = rep->FollowCamera;

  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void vtkProp3DButtonRepresentation::
ReleaseGraphicsResources(vtkWindow *win)
{
  this->Follower->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------
int vtkProp3DButtonRepresentation::
RenderVolumetricGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();

  if ( !this->CurrentProp )
    {
    return 0;
    }

  if ( this->FollowCamera )
    {
    return this->Follower->RenderVolumetricGeometry(viewport);
    }
  else
    {
    return this->CurrentProp->RenderVolumetricGeometry(viewport);
    }
}

//----------------------------------------------------------------------
int vtkProp3DButtonRepresentation::
RenderOpaqueGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();

  if ( !this->CurrentProp )
    {
    return 0;
    }

  if ( this->FollowCamera )
    {
    return this->Follower->RenderOpaqueGeometry(viewport);
    }
  else
    {
    return this->CurrentProp->RenderOpaqueGeometry(viewport);
    }
}

//-----------------------------------------------------------------------------
int vtkProp3DButtonRepresentation::
RenderTranslucentPolygonalGeometry(vtkViewport *viewport)
{
  this->BuildRepresentation();

  if ( !this->CurrentProp )
    {
    return 0;
    }

  if ( this->FollowCamera )
    {
    return this->Follower->RenderTranslucentPolygonalGeometry(viewport);
    }
  else
    {
    return this->CurrentProp->RenderTranslucentPolygonalGeometry(viewport);
    }
}
//-----------------------------------------------------------------------------
int vtkProp3DButtonRepresentation::
HasTranslucentPolygonalGeometry()
{
  this->BuildRepresentation();

  if ( this->CurrentProp )
    {
    return this->CurrentProp->HasTranslucentPolygonalGeometry();
    }
  else
    {
    return 0;
    }
}


//----------------------------------------------------------------------
double *vtkProp3DButtonRepresentation::GetBounds()
{
  if ( !this->CurrentProp )
    {
    return NULL;
    }

  if ( this->FollowCamera )
    {
    return this->Follower->GetBounds();
    }
  else
    {
    return this->CurrentProp->GetBounds();
    }
}

//----------------------------------------------------------------------
void vtkProp3DButtonRepresentation::GetActors(vtkPropCollection *pc)
{
  if ( this->CurrentProp )
    {
    this->CurrentProp->GetActors(pc);
    }
}

//----------------------------------------------------------------------
void vtkProp3DButtonRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Follow Camera: " << (this->FollowCamera ? "On\n" : "Off\n");

  os << indent << "3D Props: \n";
  vtkPropArrayIterator iter;
  int i;
  for ( i=0, iter=this->PropArray->begin();
        iter != this->PropArray->end(); ++iter, ++i )
    {
    os << indent << "  (" << i << "): " << (*iter).second.Prop << "\n";
    }
}
