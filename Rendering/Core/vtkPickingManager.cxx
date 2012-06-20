/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureUnitManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// VTK includes
#include "vtkAbstractPicker.h"
#include "vtkCamera.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPickingManager.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTimeStamp.h"

// STL includes
#include <algorithm>
#include <limits>
#include <map>
#include <vector>

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPickingManager);

//------------------------------------------------------------------------------
class vtkPickingManager::vtkInternal
{
public:
  vtkInternal(vtkPickingManager* external);
  ~vtkInternal();

  // Callback used to update the current time
  // of the manager when an event occurs in the RenderWindowInteractor.
  // Time is used to know if the cached information is still valid or obsolete.
  static void UpdateTime(vtkObject *caller,
                         unsigned long event,
                         void *clientData,
                         void *callData);

  // Select the best picker based on various criterias such as z-depth,
  // 2D overlay and/or distance to picked point
  vtkAbstractPicker* SelectPicker();

  // Compute the selection given the distance to the camera
  vtkAbstractPicker* ComputePickerSelection(double X, double Y, double Z,
                                            vtkRenderer* renderer);

  // Check if a given Observator is associated with a given Picker
  bool IsObjectLinked(vtkAbstractPicker* picker, vtkObject* object);

  // Create a new list of associated observers
  void CreateDefaultCollection(vtkAbstractPicker* picker, vtkObject* object);

  // Instead of a vtkCollection we are using a vector of a vtkSmartPointer
  // containing vtkObject to allow using 0 as a valid value because it is
  // allowed the return a picker event if he is not associated to a specific
  // object.
  // This is related with the capacity when a picker associated with a given
  // object does not manage others object,
  // it will automatically be removed from the list as well.
  typedef std::vector<vtkSmartPointer<vtkObject> > CollectionType;

  struct less_smartPtrPicker
  {
    bool operator () (const vtkSmartPointer<vtkAbstractPicker>& first,
                      const vtkSmartPointer<vtkAbstractPicker>& second) const
      {
      return first.GetPointer() < second.GetPointer();
      }
  };

  typedef std::map<vtkSmartPointer<vtkAbstractPicker>,
                   CollectionType, less_smartPtrPicker > PickerObjectsType;

  typedef std::pair<vtkSmartPointer<vtkAbstractPicker>,
                    CollectionType> PickerObjectsPairType;

  // Associate a given vtkObject to a particular picker.
  void LinkPickerObject(const PickerObjectsType::iterator& it,
                        vtkObject* object);

  // Predicate comparing a vtkAbstractPicker*
  // and a vtkSmartPointer<vtkAbstractPicker> using the PickerObjectsType.
  struct equal_smartPtrPicker
  {
    equal_smartPtrPicker(vtkAbstractPicker* picker) : Picker(picker) {}

    bool operator () (const PickerObjectsPairType& pickerObjs) const
      {
      return this->Picker == pickerObjs.first.GetPointer();
      }

    vtkAbstractPicker* Picker;
  };

  // Predicate comparing a vtkObject*
  // and a vtkSmartPointer<vtkObject> using the PickerObjectsType.
  struct equal_smartPtrObject
  {
    equal_smartPtrObject(vtkObject* object) : Object(object) {}

    bool operator () (const vtkSmartPointer<vtkObject>& smartObj) const
      {
      return this->Object == smartObj.GetPointer();
      }

    vtkObject* Object;
  };

  PickerObjectsType Pickers;           // Map the picker with the objects
  vtkTimeStamp CurrentInteractionTime; // Time of the last interaction event
  vtkTimeStamp LastPickingTime;        // Time of the last picking process
  vtkSmartPointer<vtkAbstractPicker> LastSelectedPicker;

  // Define callback to keep track of the CurrentTime and the LastPickingTime.
  // The timeStamp is use to avoid repeating the picking process if the
  // vtkWindowInteractor has not been modified, it is a huge optimization
  // avoiding each picker to relaunch the whole mechanisme to determine which
  // picker has been selected at a state of the rendering.
  vtkSmartPointer<vtkCallbackCommand> TimerCallback;

  vtkPickingManager*  External;
};

//------------------------------------------------------------------------------
// vtkInternal methods

//------------------------------------------------------------------------------
vtkPickingManager::vtkInternal::vtkInternal(vtkPickingManager* external)
{
  this->External = external;

  this->TimerCallback = vtkSmartPointer<vtkCallbackCommand>::New();
  this->TimerCallback->SetClientData(this);
  this->TimerCallback->SetCallback(UpdateTime);

}

//------------------------------------------------------------------------------
vtkPickingManager::vtkInternal::~vtkInternal()
{}

//------------------------------------------------------------------------------
void vtkPickingManager::vtkInternal::
CreateDefaultCollection(vtkAbstractPicker* picker, vtkObject* object)
{
  CollectionType objects;
  objects.push_back(object);

  this->Pickers.insert(PickerObjectsPairType(picker, objects));
}

//------------------------------------------------------------------------------
void vtkPickingManager::vtkInternal::
LinkPickerObject(const PickerObjectsType::iterator& it, vtkObject* object)
{
  CollectionType::iterator itObj = std::find_if(it->second.begin(),
                                                it->second.end(),
                                                equal_smartPtrObject(object));

  if (itObj != it->second.end() && object)
    {
    vtkDebugWithObjectMacro(
      this->External, "vtkPickingtManager::Internal::LinkPickerObject: "
      << "Current object already linked with the given picker.");

    return;
    }

  it->second.push_back(object);
}

//------------------------------------------------------------------------------
bool vtkPickingManager::vtkInternal::
IsObjectLinked(vtkAbstractPicker* picker, vtkObject* obj)
{
  if(!picker || !obj)
    {
    return false;
    }

  PickerObjectsType::iterator itPick = std::find_if(
    this->Pickers.begin(), this->Pickers.end(), equal_smartPtrPicker(picker));
  if(itPick == this->Pickers.end())
    {
    return false;
    }

  CollectionType::iterator itObj = std::find_if(itPick->second.begin(),
                                                itPick->second.end(),
                                                equal_smartPtrObject(obj));
  return (itObj != itPick->second.end());
}

//------------------------------------------------------------------------------
vtkAbstractPicker* vtkPickingManager::vtkInternal::SelectPicker()
{
  if (!this->External->Interactor)
    {
    return 0;
    }
  else if (this->External->GetOptimizeOnInteractorEvents() &&
           this->CurrentInteractionTime.GetMTime() == this->LastPickingTime)
    {
    return this->LastSelectedPicker;
    }

  // Get the event position
  double X = this->External->Interactor->GetEventPosition()[0];
  double Y = this->External->Interactor->GetEventPosition()[1];

  // Get the poked renderer
  vtkRenderer* renderer = this->External->Interactor->FindPokedRenderer(X, Y);
  vtkAbstractPicker* selectedPicker =
    this->ComputePickerSelection(X, Y, 0., renderer);

  // Keep track of the lastet picker choosen & last picking time.
  this->LastSelectedPicker = selectedPicker;
  this->LastPickingTime = this->CurrentInteractionTime;

  return selectedPicker;
}

//------------------------------------------------------------------------------
vtkAbstractPicker* vtkPickingManager::vtkInternal::
ComputePickerSelection(double X, double Y, double Z, vtkRenderer* renderer)
{
  if (!renderer)
    {
    return 0;
    }

  vtkAbstractPicker* closestPicker = 0;
  double* camPos = renderer->GetActiveCamera()->GetPosition();
  double smallestDistance2 = std::numeric_limits<double>::max();

  for(PickerObjectsType::iterator it = this->Pickers.begin();
      it != this->Pickers.end(); ++it)
    {
    int pickResult = it->first->Pick(X, Y, Z, renderer);
    double* pPos = it->first->GetPickPosition();

    if(pickResult > 0) // Keep closest object picked.
      {
      double distance2 = vtkMath::Distance2BetweenPoints(camPos, pPos);

      if(smallestDistance2 > distance2)
        {
        smallestDistance2 = distance2;
        closestPicker = it->first;
        }
      }
    }

  return closestPicker;
}

//------------------------------------------------------------------------------
void vtkPickingManager::vtkInternal::UpdateTime(vtkObject *vtkNotUsed(caller),
                                                unsigned long vtkNotUsed(event),
                                                void *clientData,
                                                void *vtkNotUsed(calldata))
{
  vtkPickingManager::vtkInternal* self =
    reinterpret_cast<vtkPickingManager::vtkInternal*>(clientData);
  if (!self)
    {
    return;
    }

  self->CurrentInteractionTime.Modified();
}

//------------------------------------------------------------------------------
// vtkPickingManager methods

//------------------------------------------------------------------------------
vtkPickingManager::vtkPickingManager()
{
  this->Internal = new vtkInternal(this);
  this->Interactor = 0;

  this->Enabled = true;
  this->OptimizeOnInteractorEvents = true;
}

//------------------------------------------------------------------------------
vtkPickingManager::~vtkPickingManager()
{
  this->SetInteractor(0);
  delete this->Internal;
}

//------------------------------------------------------------------------------
void vtkPickingManager::SetInteractor(vtkRenderWindowInteractor* rwi)
{
  if (rwi == this->Interactor)
    {
    return;
    }
  if (this->Interactor)
    {
    this->Interactor->RemoveObserver(this->Internal->TimerCallback);
    }

  this->Interactor = rwi;
  if (this->Interactor)
    {
    this->Interactor->AddObserver(
      vtkCommand::ModifiedEvent, this->Internal->TimerCallback);
    }

  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPickingManager::SetOptimizeOnInteractorEvents(bool optimize)
{
  if (this->OptimizeOnInteractorEvents == optimize || !this->Interactor)
    {
    return;
    }

  this->OptimizeOnInteractorEvents = optimize;
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkPickingManager::AddPicker(vtkAbstractPicker* picker,
                                  vtkObject* object)
{
  if (!picker)
    {
    return;
    }

  // Linke the object if the picker is already registered
  vtkPickingManager::vtkInternal::PickerObjectsType::iterator it =
    std::find_if( this->Internal->Pickers.begin(),
                  this->Internal->Pickers.end(),
                  vtkPickingManager::vtkInternal::equal_smartPtrPicker(picker));

  if (it != this->Internal->Pickers.end() )
    {
    vtkDebugMacro("vtkPickingtManager::AddPicker: "
      << "Picker already in the manager, the object will be linked");

    this->Internal->LinkPickerObject(it, object);
    return;
    }

  // The picker does not exists in the manager yet.
  // Create the list of associated objects
  this->Internal->CreateDefaultCollection(picker, object);
}

//------------------------------------------------------------------------------
void vtkPickingManager::RemovePicker(vtkAbstractPicker* picker,
                                     vtkObject* object)
{
  vtkPickingManager::vtkInternal::PickerObjectsType::iterator it =
    std::find_if( this->Internal->Pickers.begin(),
                  this->Internal->Pickers.end(),
                  vtkPickingManager::vtkInternal::equal_smartPtrPicker(picker));

  // The Picker does not exist
  if (it == this->Internal->Pickers.end())
    {
    return;
    }

  vtkPickingManager::vtkInternal::CollectionType::iterator itObj =
    std::find_if(it->second.begin(),
                 it->second.end(),
                 vtkPickingManager::vtkInternal::equal_smartPtrObject(object));

  // The object is not associated with the given picker
  if (itObj == it->second.end())
    {
    return;
    }

  it->second.erase(itObj);

  // We delete the picker when it is not associated with any object.
  if(it->second.size() == 0)
    {
    this->Internal->Pickers.erase(it);
    }
}

//------------------------------------------------------------------------------
void vtkPickingManager::RemoveObject(vtkObject* object)
{
  vtkPickingManager::vtkInternal::PickerObjectsType::iterator it =
    this->Internal->Pickers.begin();

  for(; it != this->Internal->Pickers.end();)
    {
    vtkPickingManager::vtkInternal::CollectionType::iterator itObj =
      std::find_if(it->second.begin(),
                   it->second.end(),
                   vtkPickingManager::vtkInternal::equal_smartPtrObject(object));

    if (itObj != it->second.end())
      {
      it->second.erase(itObj);

      if (it->second.size() == 0)
        {
        this->Internal->Pickers.erase(it);
        continue;
        }
      }

    ++it;
    }
}

//------------------------------------------------------------------------------
bool vtkPickingManager::Pick(vtkAbstractPicker* picker, vtkObject* obj)
{
  if (!this->Internal->IsObjectLinked(picker, obj))
    {
    return false;
    }

  return (this->Pick(picker));
}

//------------------------------------------------------------------------------
bool vtkPickingManager::Pick(vtkObject* obj)
{
  vtkAbstractPicker* picker = this->Internal->SelectPicker();
  if(!picker)
    {
    return false;
    }
  // If the object is not contained in the list of the associated active pickers
  // return false
  return (this->Internal->IsObjectLinked(picker, obj));
}

//------------------------------------------------------------------------------
bool vtkPickingManager::Pick(vtkAbstractPicker* picker)
{
  return (picker == this->Internal->SelectPicker());
}

//------------------------------------------------------------------------------
int vtkPickingManager::GetNumberOfPickers()
{
  return this->Internal->Pickers.size();
}

//------------------------------------------------------------------------------
int vtkPickingManager::GetNumberOfObjectsLinked(vtkAbstractPicker* picker)
{
  if (!picker)
    {
    return 0;
    }

  vtkPickingManager::vtkInternal::PickerObjectsType::iterator it =
    std::find_if( this->Internal->Pickers.begin(),
                  this->Internal->Pickers.end(),
                  vtkPickingManager::vtkInternal::equal_smartPtrPicker(picker));

  if (it == this->Internal->Pickers.end())
    {
    return 0;
    }

  return it->second.size();
}

//------------------------------------------------------------------------------
void vtkPickingManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if ( this->Interactor )
    {
    os << indent << "RenderWindowInteractor: " << this->Interactor << "\n";
    }

  os << indent <<  "Registered pickers: " << this->Internal->Pickers.size() << "\n";

  vtkPickingManager::vtkInternal::PickerObjectsType::iterator it =
    this->Internal->Pickers.begin();

  for(; it != this->Internal->Pickers.end(); ++it)
    {
    os << indent << indent << "Picker: " << it->first.GetPointer() << "\n";
    os << indent << indent << "Size associated objects: " << it->second.size()
       << "\n";
    }
}
