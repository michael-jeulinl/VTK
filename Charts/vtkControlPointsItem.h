/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkControlPointsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkControlPointsItem - Abstract class for control points items.
// .SECTION Description
// vtkControlPointsItem provides control point painting and management for
// subclasses that provide points (typically control points of a transfer
// function)
// .SECTION See Also
// vtkScalarsToColorsItem
// vtkPiecewiseControlPointsItem

#ifndef __vtkControlPointsItem_h
#define __vtkControlPointsItem_h

#include "vtkCommand.h" // For vtkCommand enum
#include "vtkPlot.h"
#include "vtkVector.h"

class vtkCallbackCommand;
class vtkContext2D;
class vtkPoints2D;
class vtkTransform2D;

class VTK_CHARTS_EXPORT vtkControlPointsItem: public vtkPlot
{
public:
  vtkTypeMacro(vtkControlPointsItem, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  enum {
    CurrentPointChangedEvent = vtkCommand::UserEvent,
    CurrentPointEditEvent
  };

  // Description:
  // Bounds of the item, typically the bound of all the control points
  // except if custom bounds have been set \sa SetUserBounds.
  virtual void GetBounds(double bounds[4]);

  // Description:
  // Set custom bounds, except if bounds are invalid, bounds will be
  // automatically computed based on the range of the control points
  // Invalid bounds by default.
  vtkSetVector4Macro(UserBounds, double);
  vtkGetVector4Macro(UserBounds, double);

  // Description:
  // Controls the valid range for the values.
  // An invalid value (0, -1, 0., -1, 0, -1.) indicates that the valid
  // range is the current bounds. It is the default behavior.
  vtkSetVector4Macro(ValidBounds, double);
  vtkGetVector4Macro(ValidBounds, double);

  // Description:
  // Paint the points with a fixed size (cosmetic) which doesn't depend
  // on the scene zoom factor. Selected and unselected points are drawn
  // with a different color.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Select a point by its ID
  void SelectPoint(vtkIdType pointId);

  // Description:
  // Utility function that selects a point providing its coordinates.
  // To be found, the position of the point must be no further away than its
  // painted point size
  void SelectPoint(double* currentPoint);

  // Description:
  // Select all the points
  void SelectAllPoints();

  // Description:
  // Unselect a point by its ID
  void DeselectPoint(vtkIdType pointId);

  // Description:
  // Utility function that unselects a point providing its coordinates.
  // To be found, the position of the point must be no further away than its
  // painted point size
  void DeselectPoint(double* currentPoint);

  // Description:
  // Unselect all the previously selected points
  void DeselectAllPoints();

  // Description:
  // Toggle the selection of a point by its ID. If the point was selected then
  // unselect it, otherwise select it.
  void ToggleSelectPoint(vtkIdType pointId);

  // Description:
  // Utility function that toggles the selection a point providing its
  // coordinates. To be found, the position of the point must be no further
  // away than its painted point size
  void ToggleSelectPoint(double* currentPoint);

  // Description:
  // Select all points in the specified rectangle.
  virtual bool SelectPoints(const vtkVector2f& min, const vtkVector2f& max);

  // Description:
  // Return the number of selected points.
  vtkIdType GetNumberOfSelectedPoints()const;

  // Description:
  // Returns the vtkIdType of the point given its coordinates and a tolerance
  // based on the screen point size.
  vtkIdType FindPoint(double* pos);

  // Description:
  // Returns true if pos is above the pointId point, false otherwise.
  // It uses the size of the drawn point. To search what point is under the pos,
  // use the more efficient \sa FindPoint() instead.
  bool IsOverPoint(double* pos, vtkIdType pointId);

  // Description:
  // Returns the id of the control point exactly matching pos, -1 if not found.
  vtkIdType GetControlPointId(double* pos);

  // Description:
  // Utility function that returns an array of all the control points IDs
  // Typically: [0, 1, 2, ... n -1] where n is the point count
  // You are responsible for deleting the array.
  vtkIdTypeArray* GetControlPointsIds()const;

  // Description:
  // Controls whether or not control points are drawn (true) or clicked and
  // moved (false).
  // False by default.
  vtkGetMacro(StrokeMode, bool);

  // Description:
  // If DrawPoints is false, SwitchPoints controls the behavior when a control
  // point is dragged past another point. The crossed point becomes current
  // (true) or the current point is blocked/stopped (false).
  // False by default.
  vtkSetMacro(SwitchPointsMode, bool);
  vtkGetMacro(SwitchPointsMode, bool);

  // Description:
  // Add a point to the function. Returns the index of the point (0 based),
  // or -1 on error.
  // Subclasses should reimplement this function to do the actual work.
  virtual vtkIdType AddPoint(double* newPos) = 0;

  // Description:
  // Remove a point of the function. Returns the index of the point (0 based),
  // or -1 on error.
  // Subclasses should reimplement this function to do the actual work.
  virtual vtkIdType RemovePoint(double* pos) = 0;

  // Description:
  // Remove a point give its id. It is a utility function that internally call
  // the virtual method RemovePoint(double*) and return its result.
  vtkIdType RemovePoint(vtkIdType pointId);

  // Description:
  // Remove the current point.
  inline void RemoveCurrentPoint();

  // Description:
  // Returns the total number of points
  virtual vtkIdType GetNumberOfPoints()const = 0;

  // Description:
  // Returns the x and y coordinates as well as the midpoint and sharpness
  // of the control point corresponding to the index.
  // point must be a double array of size 4.
  virtual void GetControlPoint(vtkIdType index, double *point)const = 0;

  // Description:
  // Sets the x and y coordinates as well as the midpoint and sharpness
  // of the control point corresponding to the index.
  virtual void SetControlPoint(vtkIdType index, double *point) = 0;

  // Description:
  // Move the points referred by pointIds by a given translation.
  // The new positions won't be outside the bounds.
  // Warning: if you pass this->GetSelection(), the array is deleted after
  // each individual point move. Increase the reference count of the array.
  void MovePoints(const vtkVector2f& translation, vtkIdTypeArray* pointIds);

  // Description:
  // Spread the points referred by pointIds
  // If factor > 0, points are moved away from each other.
  // If factor < 0, points are moved closer to each other
  // Warning: if you pass this->GetSelection(), the array is deleted after
  // each individual point move. Increase the reference count of the array.
  void SpreadPoints(float factor, vtkIdTypeArray* pointIds);

  // Description:
  // Returns the current point ID selected or -1 if there is no point current.
  // No current point by default.
  vtkIdType GetCurrentPoint()const;

  // Description:
  // Sets the current point selected.
  void SetCurrentPoint(vtkIdType index);

  // Description:
  // Gets the selected point pen and brush.
  vtkGetObjectMacro(SelectedPointPen, vtkPen);

  // Description:
  // Depending on the control points item, the brush might not be taken into
  // account.
  vtkGetObjectMacro(SelectedPointBrush, vtkBrush);

  // Description:
  // Recompute the bounds next time they are requested.
  // You shouldn't have to call it but it is provided for rare cases.
  void ResetBounds();

  virtual bool KeyPressEvent(const vtkContextKeyEvent &key);
  virtual bool KeyReleaseEvent(const vtkContextKeyEvent &key);

protected:
  vtkControlPointsItem();
  virtual ~vtkControlPointsItem();

  virtual void StartChanges()=0;
  virtual void EndChanges()=0;

  static void CallComputePoints(vtkObject* sender, unsigned long event, void* receiver, void* params);

  // Description:
  // Must be reimplemented by subclasses to calculate the points to draw.
  // It's subclass responsibility to call ComputePoints() via the callback
  virtual void ComputePoints();
  virtual unsigned long int GetControlPointsMTime() =0;

  // Description:
  // Returns true if the supplied x, y coordinate is on a control point.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Clamp the given 2D pos into the bounds of the function.
  // Return true if the pos has been clamped, false otherwise.
  bool ClampPos(double pos[2], double bounds[4]);
  bool ClampValidPos(double pos[2]);

  // Description:
  // Internal function that paints a collection of points and optionally
  // excludes some.
  void DrawUnselectedPoints(vtkContext2D* painter);
  void DrawSelectedPoints(vtkContext2D* painter);
  virtual void DrawPoint(vtkContext2D* painter, vtkIdType index);

  // Description:
  // Mouse button down event.
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);
  virtual bool MouseDoubleClickEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  void SetCurrentPointPos(const vtkVector2f& newPos);
  vtkIdType SetPointPos(vtkIdType point, const vtkVector2f& newPos);
  void MoveCurrentPoint(const vtkVector2f& translation);
  vtkIdType MovePoint(vtkIdType point, const vtkVector2f& translation);

  inline vtkVector2f GetSelectionCenterOfMass()const;
  vtkVector2f GetCenterOfMass(vtkIdTypeArray* pointIDs)const;

  void Stroke(const vtkVector2f& newPos);
  virtual void EditPoint(float vtkNotUsed(tX), float vtkNotUsed(tY));
  // Description:
  // Mouse button release event.
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

  void AddPointId(vtkIdType addedPointId);

  vtkCallbackCommand* Callback;
  vtkPen*             SelectedPointPen;
  vtkBrush*           SelectedPointBrush;
  int                 BlockUpdates;
  vtkIdType           CurrentPoint;

  double              Bounds[4];
  double              UserBounds[4];
  double              ValidBounds[4];

  vtkTransform2D*     Transform;
  float               ScreenPointRadius;

  bool                StrokeMode;
  bool                SwitchPointsMode;
  bool                MouseMoved;
  bool                EnforceValidFunction;
  vtkIdType           PointToDelete;
  bool                PointAboutToBeDeleted;
  vtkIdType           PointToToggle;
  bool                PointAboutToBeToggled;
  bool                InvertShadow;
private:
  vtkControlPointsItem(const vtkControlPointsItem &); // Not implemented.
  void operator=(const vtkControlPointsItem &);   // Not implemented.

  void      ComputeBounds();
  void      ComputeBounds(double* bounds);

  vtkIdType RemovePointId(vtkIdType removedPointId);
};

//-----------------------------------------------------------------------------
void vtkControlPointsItem::RemoveCurrentPoint()
{
  this->RemovePoint(this->GetCurrentPoint());
}

//-----------------------------------------------------------------------------
vtkVector2f vtkControlPointsItem::GetSelectionCenterOfMass()const
{
  return this->GetCenterOfMass(this->Selection);
}

#endif
