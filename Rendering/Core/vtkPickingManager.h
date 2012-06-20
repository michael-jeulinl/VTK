/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureUnitManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkPickingManager_h
#define __vtkPickingManager_h

#include "vtkObject.h"
#include "vtkRenderingCoreModule.h" // For export macro

class vtkAbstractPicker;
class vtkRenderWindowInteractor;

class VTKRENDERINGCORE_EXPORT vtkPickingManager : public vtkObject
{
public:
  static vtkPickingManager *New();
  vtkTypeMacro(vtkPickingManager,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enable/Disable managment. By default pickinManagers are enabled when
  // initialized.
  vtkBooleanMacro(Enabled, bool);
  vtkSetMacro(Enabled, bool);
  vtkGetMacro(Enabled, bool);

  // Description:
  // Enable/Disable optimization depending on the renderWindowInteractor events.
  // By default pickinManagers does not use the optimization.
  void SetOptimizeOnInteractorEvents(bool optimize);
  vtkGetMacro(OptimizeOnInteractorEvents, bool);

  // Description
  // Set the windowInteractor associated with the manager.
  void SetInteractor(vtkRenderWindowInteractor* iren);
  vtkGetMacro(Interactor, vtkRenderWindowInteractor*);

  // Add the picker to the managed process. [option] associate a given
  // vtkObject with the picker (i.e. either vtkInteractorObserver or
  // vtkWidgetRepresentation).
  // Important a null object is different from ont to an other !!
  // This has been done to allow adding two times the same picker to the manager
  // by not passing the object reference and when the not forcing the supression
  // of the picker for all.
  void AddPicker(vtkAbstractPicker* picker, vtkObject* object = 0);

  // Remove the picker from the managed process.
  void RemovePicker(vtkAbstractPicker* picker, vtkObject* object = 0);

  // Remove all occurences of the object from the list of the managed ones
  // If one of the picker associated with the given object does not manage
  // others object, it will automatically be removed from the list as well.
  void RemoveObject(vtkObject* object);

  // Run Pickings and return if the vtkObject associated with the
  // picker passed in argument is the one selected.
  bool Pick(vtkAbstractPicker*, vtkObject* object);

  // Run Pickings and return if the vtkObject is the one selected
  bool Pick(vtkObject* object);

  // Run Pickings and return if the picker is the one mediated
  bool Pick(vtkAbstractPicker* picker);

  // Return the number of pickers registered.
  int GetNumberOfPickers();

  // Return the number of object linked with a given picker.
  // A null object is counted as an associated object.
  int GetNumberOfObjectsLinked(vtkAbstractPicker* picker);

protected:
  vtkPickingManager();
  ~vtkPickingManager();

  // Used to associate the manager with the interactor
  vtkRenderWindowInteractor* Interactor;
  bool Enabled;
  bool OptimizeOnInteractorEvents;

private:
  vtkPickingManager(const vtkPickingManager&);  // Not implemented.
  void operator=(const vtkPickingManager&);     // Not implemented.

  class vtkInternal;
  vtkInternal* Internal;
};

#endif
