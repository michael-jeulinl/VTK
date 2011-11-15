/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolarAxesActor.cxx
  Thanks:    Kathleen Bonnell, B Division, Lawrence Livermore National Lab

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkPolarAxesActor.h"

#include "vtkAxisActor.h"
#include "vtkAxisFollower.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkFollower.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkStringArray.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"

#include <vtksys/ios/sstream>

vtkStandardNewMacro(vtkPolarAxesActor);
vtkCxxSetObjectMacro(vtkPolarAxesActor, Camera,vtkCamera);
vtkCxxSetObjectMacro(vtkPolarAxesActor,PolarAxisLabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkPolarAxesActor,PolarAxisTitleTextProperty,vtkTextProperty);

// ******************************************************************
void vtkPolarAxesActor::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os,indent );

  os << indent << "Bounds: \n";
  os << indent << "  Xmin,Xmax: (" << this->Bounds[0] << ", "
     << this->Bounds[1] << " )\n";
  os << indent << "  Ymin,Ymax: (" << this->Bounds[2] << ", "
     << this->Bounds[3] << " )\n";
  os << indent << "  Zmin,Zmax: (" << this->Bounds[4] << ", "
     << this->Bounds[5] << " )\n";

  os << indent << "ScreenSize: (" << this->ScreenSize << ")\n";

  os << indent << "Number Of Radial Axes" << this->NumberOfRadialAxes << endl;

  os << indent << "Pole: (" 
     << this->Pole[0] << ", "
     << this->Pole[1] << ", "
     << this->Pole[2] << " )\n";

  os << indent << "Maximum Radius" << this->MaximumRadius << endl;
  os << indent << "Auto-Scale Radius" << this->AutoScaleRadius << endl;
  os << indent << "Maximum Angle" << this->MaximumAngle << endl;
  os << indent << "Radial Units (degrees): "
     << ( this->RadialUnits ? "On\n" : "Off\n" ) << endl;

  if ( this->Camera )
    {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf( os,indent.GetNextIndent() );
    }
  else
    {
    os << indent << "Camera: (none)\n";
    }

  os << indent << "Rebuild Axes: "
     << ( this->RebuildAxes ? "On\n" : "Off\n" );


  os << indent << "Polar Axis Title: " << this->PolarAxisTitle << "\n";
  os << indent << "PolarAxisLabelTextProperty: " << this->PolarAxisLabelTextProperty << endl;
  os << indent << "PolarAxisTitleTextProperty: " << this->PolarAxisTitleTextProperty << endl;

  os << indent << "Radial Axes Visibility: "
     << ( this->RadialAxesVisibility ? "On\n" : "Off\n" );

  os << indent << "Radial Title Visibility: "
     << ( this->RadialTitleVisibility ? "On" : "Off" ) << endl;

  os << indent << "Polar Label Visibility: "
     << ( this->PolarLabelVisibility ? "On" : "Off" ) << endl;

  os << indent << "Polar Tick Visibility: "
     << ( this->PolarTickVisibility ? "On" : "Off" ) << endl;

  os << indent << "Radial Axes Label Format: " << this->RadialLabelFormat << "\n";

  os << indent << "Tick Location: " << this->TickLocation << endl;
}

// *************************************************************************
vtkPolarAxesActor::vtkPolarAxesActor() : vtkActor()
{
  // Default bounds
  this->Bounds[0] = -1.0; this->Bounds[1] = 1.0;
  this->Bounds[2] = -1.0; this->Bounds[3] = 1.0;
  this->Bounds[4] = -1.0; this->Bounds[5] = 1.0;

  // Default pole coordinates
  this->Pole[0] = VTK_DOUBLE_MAX;
  this->Pole[1] = VTK_DOUBLE_MAX;
  this->Pole[2] = VTK_DOUBLE_MAX;

  // Default number of polar axes
  this->NumberOfRadialAxes = VTK_MAXIMUM_NUMBER_OF_RADIAL_AXES;

  // Default maximum polar radius
  this->MaximumRadius = VTK_DOUBLE_MAX;

  // Do not auto-scale radius by default
  this->AutoScaleRadius = false;

  // Default maximum polar angle
  this->MaximumAngle = VTK_DEFAULT_MAXIMUM_POLAR_ANGLE;

  // By default show angle units (degrees)
  this->RadialUnits = 1;

  this->RebuildAxes = false;

  this->Camera = NULL;

  this->ScreenSize = 10.0;

  // Considering pivot point at center of the geometry,
  // hence ( this->ScreenSize * 0.5 ).
  this->LabelScreenOffset = 20.0 + this->ScreenSize * 0.5;

  // Properties of the radial axes
  this->RadialAxesProperty = vtkProperty::New();

  // Text properties of polar axis title and labels, with default red color
  this->PolarAxisTitleTextProperty = vtkTextProperty::New();
  this->PolarAxisTitleTextProperty->SetColor( 1., 0. ,0. );
  this->PolarAxisTitleTextProperty->SetFontFamilyToArial();
  this->PolarAxisLabelTextProperty = vtkTextProperty::New();
  this->PolarAxisLabelTextProperty->SetColor( .8, 0. ,0. );
  this->PolarAxisLabelTextProperty->SetFontFamilyToArial();

  // Create and set radial axes
  this->RadialAxes = new vtkAxisActor*[VTK_MAXIMUM_NUMBER_OF_RADIAL_AXES];
  for ( int i = 0; i < VTK_MAXIMUM_NUMBER_OF_RADIAL_AXES; ++ i )
    {
    // Create axis of type X
    this->RadialAxes[i] = vtkAxisActor::New();
    vtkAxisActor* axis = this->RadialAxes[i];
    axis->SetAxisTypeToX();

    // Title offset is auto-calculated for now
    axis->SetCalculateTitleOffset( 1 );
    axis->SetCalculateLabelOffset( 0 );

    // Pass information to axes followers
    vtkAxisFollower* follower = axis->GetTitleActor();
    follower->SetAxis( axis );
    if ( ! i )
      {
      // Replace default screen offset (for polar axis only)
      follower->SetScreenOffset( this->LabelScreenOffset + this->ScreenSize * 0.5 );
      }
    }

  // Default tick location, defined in vtkAxisActor
  this->TickLocation = VTK_TICKS_BOTH;

  // By default all features are visible
  this->RadialAxesVisibility = 1;
  this->RadialTitleVisibility = 1;
  this->PolarLabelVisibility = 1;
  this->PolarTickVisibility = 1;

  // Default title for polar axis (can also be called "Radius")
  this->PolarAxisTitle = new char[16];
  sprintf(this->PolarAxisTitle, "%s", "Radial Distance");

  this->RadialLabelFormat = new char[8];
  sprintf( this->RadialLabelFormat, "%s", "%-#6.3g" );

  this->RenderCount = 0;

  this->RenderSomething = 0;

  this->UserRadialPow = 0;

  this->AutoLabelScaling = true;

  this->LabelScale = -1.0;
  this->TitleScale = -1.0;
}

// ****************************************************************************
vtkPolarAxesActor::~vtkPolarAxesActor()
{
  this->SetCamera( NULL );

  if ( this->RadialAxesProperty )
    {
    this->RadialAxesProperty->Delete();
    }

  if ( this->RadialLabelFormat )
    {
    delete [] this->RadialLabelFormat;
    this->RadialLabelFormat = NULL;
    }

  if ( this->PolarAxisTitle )
    {
    delete [] this->PolarAxisTitle;
    this->PolarAxisTitle = NULL;
    }

  if ( this->PolarAxisTitleTextProperty )
    {
    this->PolarAxisTitleTextProperty->Delete();
    this->PolarAxisTitleTextProperty = NULL;
    }

  if ( this->PolarAxisLabelTextProperty )
    {
    this->PolarAxisLabelTextProperty->Delete();
    this->PolarAxisLabelTextProperty = NULL;
    }

  if ( this->RadialAxes )
    {
    for ( int i = 0; i < VTK_MAXIMUM_NUMBER_OF_RADIAL_AXES; ++ i )
      {
      if ( this->RadialAxes[i] )
        {
        this->RadialAxes[i]->Delete();
        this->RadialAxes[i] = NULL;
        }
      }
    delete [] this->RadialAxes;
    this->RadialAxes = NULL;
    }
}

// ****************************************************************************
void vtkPolarAxesActor::ShallowCopy( vtkPolarAxesActor *actor )
{
  this->Superclass::ShallowCopy( actor );

  this->SetBounds( actor->GetBounds() );
  this->SetPole( actor->GetPole() );
  this->SetMaximumRadius( actor->GetMaximumRadius() );
  this->SetAutoScaleRadius( actor->GetAutoScaleRadius() );
  this->SetRadialUnits( actor->GetRadialUnits() );
  this->SetRadialLabelFormat( actor->GetRadialLabelFormat() );
  this->SetMaximumAngle( actor->GetMaximumAngle() );
  
  this->SetCamera( actor->GetCamera() );

  this->SetPolarAxisTitle( actor->GetPolarAxisTitle() );
  this->LabelScreenOffset = actor->LabelScreenOffset;
  this->ScreenSize        = actor->ScreenSize;
  this->LabelScreenOffset = actor->LabelScreenOffset;
  this->SetRadialLabelFormat( actor->GetRadialLabelFormat() );
  this->SetPolarAxisTitleTextProperty( actor->GetPolarAxisTitleTextProperty() );
  this->SetPolarAxisLabelTextProperty( actor->GetPolarAxisLabelTextProperty() );
}

// *************************************************************************
int vtkPolarAxesActor::RenderOpaqueGeometry( vtkViewport *viewport )
{
  // Initialization
  static bool initialRender = true;
  if ( !this->Camera )
    {
    vtkErrorMacro( <<"No camera!" );
    this->RenderSomething = 0;
    return 0;
    }

  this->BuildAxes( viewport );

  if ( initialRender || this->RebuildAxes )
    {
    for ( int i = 0; i < this->NumberOfRadialAxes; ++ i )
      {
      this->RadialAxes[i]->BuildAxis( viewport, true );
      }
    }
  initialRender = false;
  this->RebuildAxes = false;

  //Render the axes
  int renderedSomething = 0;
  if ( this->RadialAxesVisibility )
    {
    for ( int i = 0; i < this->NumberOfRadialAxes; ++ i )
      {
      renderedSomething +=
        this->RadialAxes[i]->RenderOpaqueGeometry( viewport );
      }
    }

  return renderedSomething;
}

// *************************************************************************
// Screen size affects the screen offset as well.
// *************************************************************************
void vtkPolarAxesActor::SetScreenSize( double screenSize )
{
  this->ScreenSize = screenSize;
  // Considering pivot point at center of the geometry,
  // hence ( this->ScreenSize * 0.5 ).
  this->LabelScreenOffset = 20.0 + this->ScreenSize * 0.5;

  for ( int i = 0; i < this->NumberOfRadialAxes; ++ i )
    {
    vtkAxisActor* axis = this->RadialAxes[i];
  
    int numberOfLabelsBuilt = axis->GetNumberOfLabelsBuilt();
    vtkAxisFollower** labelActors = axis->GetLabelActors();
    for( int k=0; k < numberOfLabelsBuilt; ++k )
      {
      labelActors[k]->SetScreenOffset( this->LabelScreenOffset );
      }
    }

  this->Modified();
}

// *************************************************************************
void vtkPolarAxesActor::ReleaseGraphicsResources( vtkWindow *win )
{
  for ( int i = 0; i < this->NumberOfRadialAxes;  ++i )
    {
    this->RadialAxes[i]->ReleaseGraphicsResources( win );
    }
}

// *************************************************************************
void vtkPolarAxesActor::GetBounds( double bounds[6])
{
  for ( int i=0; i< 6; i++)
    {
    bounds[i] = this->Bounds[i];
    }
}

// *************************************************************************
void vtkPolarAxesActor::GetBounds( double& xmin, double& xmax,
                                 double& ymin, double& ymax,
                                 double& zmin, double& zmax )
{
  xmin = this->Bounds[0];
  xmax = this->Bounds[1];
  ymin = this->Bounds[2];
  ymax = this->Bounds[3];
  zmin = this->Bounds[4];
  zmax = this->Bounds[5];
}

// *************************************************************************
double *vtkPolarAxesActor::GetBounds()
{
  return this->Bounds;
}

// *************************************************************************
void vtkPolarAxesActor::TransformBounds( vtkViewport *viewport,
                                         double bounds[6] )
{
  double minPt[3], maxPt[3], transMinPt[3], transMaxPt[3];
  minPt[0] = this->Bounds[0];
  minPt[1] = this->Bounds[2];
  minPt[2] = this->Bounds[4];
  maxPt[0] = this->Bounds[1];
  maxPt[1] = this->Bounds[3];
  maxPt[2] = this->Bounds[5];

  viewport->SetWorldPoint(minPt[0], minPt[1], minPt[2], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transMinPt);
  viewport->SetWorldPoint(maxPt[0], maxPt[1], maxPt[2], 1.0);
  viewport->WorldToDisplay();
  viewport->GetDisplayPoint(transMaxPt);

  bounds[0] = transMinPt[0];
  bounds[2] = transMinPt[1];
  bounds[4] = transMinPt[2];
  bounds[1] = transMaxPt[0];
  bounds[3] = transMaxPt[1];
  bounds[5] = transMaxPt[2];
}

// ****************************************************************************
//  Method: LabelExponent
//
//  Purpose:
//      Determines the proper exponent for the min and max values.
//
//  Arguments:
//      min     The minimum value along a certain axis.
//      max     The maximum value along a certain axis.
//
//  Note:       This code is mostly stolen from old MeshTV code,
//              /meshtvx/toolkit/plotgrid.c, axlab_format.
// ****************************************************************************
int vtkPolarAxesActor::LabelExponent( double min, double max )
{
  if ( min == max )
    {
    return 0;
    }

  //
  // Determine power of 10 to scale axis labels to.
  //
  double range = ( fabs( min ) > fabs( max ) ? fabs( min ) : fabs( max ) );
  double pow10 = log10( range );

  //
  // Cutoffs for using scientific notation.  The following 4 variables
  // should all be static for maximum performance but were made non-static
  // to get around a compiler bug with the MIPSpro 7.2.1.3 compiler.
  //
  double eformat_cut_min = -1.5;
  double eformat_cut_max =  3.0;
  double cut_min = pow( 10., eformat_cut_min );
  double cut_max = pow( 10., eformat_cut_max );
  double ipow10;
  if ( range < cut_min || range > cut_max )
    {
    //
    // We are going to use scientific notation and round the exponents to
    // the nearest multiple of three.
    //
    ipow10 = ( floor( floor( pow10 )/3.) )*3;
    }
  else
    {
    ipow10 = 0;
    }

  return static_cast<int>( ipow10 );
}

// *************************************************************************
void vtkPolarAxesActor::BuildAxes( vtkViewport *viewport )
{
  double bounds[6];

  if ( ( this->GetMTime() < this->BuildTime.GetMTime() ))
    {
    this->AutoScale( viewport );
    return;
    }

  this->SetNonDependentAttributes();

  // Determine the bounds for possible use ( input, prop, or user-defined )
  this->GetBounds( bounds );
  
  // If pole coordinates are invalid, use bounds
  double o[3];
  for ( int i = 0; i < 3; ++ i )
    {
    o[i] = this->Pole[i] == VTK_DOUBLE_MAX ? bounds[i * 2] : this->Pole[i];
    }
  
  // If axial scale it out of proportions with object length scale, reset to ls
  double ls = fabs( bounds[1] -  bounds[0] ) + fabs( bounds[3] -  bounds[2] );
  if ( this->AutoScaleRadius
       || this->MaximumRadius < 1.e-6 * ls 
       || this->MaximumRadius > 1.e6 * ls )
    {
    this->MaximumRadius = .5 * ls;
    }

  // Prepare axes for rendering with user-definable options
  double dAlpha =  this->MaximumAngle / ( this->NumberOfRadialAxes - 1. );

  // Set radial axes
  for ( int i = 0; i < this->NumberOfRadialAxes;  ++ i )
    {
    double theta = i * dAlpha;
    double thetaRad = vtkMath::RadiansFromDegrees( theta );
    vtkAxisActor* axis = this->RadialAxes[i];
    double x = o[0] + this->MaximumRadius * cos( thetaRad );
    double y = o[1] + this->MaximumRadius * sin( thetaRad );
    axis->GetPoint1Coordinate()->SetValue( o[0], o[1], o[2] );
    axis->GetPoint2Coordinate()->SetValue( x, y, o[2] );

    // Set axis ticks
    axis->SetRange( 0., this->MaximumRadius );
    axis->SetMajorTickSize( .02 * this->MaximumRadius );

    // Set axis title
    if ( i )
      {
      // Use polar angle as title for non-polar axes
      vtksys_ios::ostringstream thetaStream;
      thetaStream << theta;
      if ( this->RadialUnits )
        {
        thetaStream << " deg.";
        }

      axis->SetTitle( thetaStream.str().c_str() );
      }
    else // if ( i ) 
      {
      // Special case of polar axis
      axis->SetTitle( this->PolarAxisTitle );
      }
    }
  
  // Build polar axis tick, with respect to specified x-origin
  this->BuildPolarAxisTicks( o[0] );

  // Build polar axis labels with 0.01 zero-threshold for labels
  this->BuildPolarAxisLabels( .01 );

  // Scale appropriately.
  this->AutoScale( viewport );

  this->RenderSomething = 1;
  this->BuildTime.Modified();
}

// *************************************************************************
void vtkPolarAxesActor::SetNonDependentAttributes()
{
  vtkProperty *prop = this->GetProperty();
  prop->SetAmbient( 1.0 );
  prop->SetDiffuse( 0.0 );
  for ( int i = 0; i < this->NumberOfRadialAxes; ++ i )
    {
    vtkAxisActor* axis = this->RadialAxes[i];
    axis->SetCamera( this->Camera );
    axis->SetProperty( prop );
    axis->SetAxisLinesProperty( this->RadialAxesProperty );
    axis->SetTickLocation( this->TickLocation );
    axis->SetBounds( this->Bounds );
    axis->SetAxisVisibility( this->RadialAxesVisibility );
    axis->SetTitleVisibility( this->RadialTitleVisibility );
    axis->SetMinorTicksVisible( 0 );
    // Features available only on polar axis
    if ( ! i )
      {
      axis->SetLabelVisibility( this->PolarLabelVisibility );
      axis->SetTickVisibility( this->PolarTickVisibility );
      axis->SetTitleTextProperty( this->GetPolarAxisTitleTextProperty() );
      axis->SetLabelTextProperty( this->GetPolarAxisLabelTextProperty() );
      }
    else
      {
      axis->SetLabelVisibility( 0 );
      axis->SetTickVisibility( 0 );
      axis->GetTitleTextProperty()->SetColor( this->GetRadialAxesProperty()->GetColor() );
      }
    }
}

// *************************************************************************
double vtkPolarAxesActor::MaxOf( double a, double b )
{
  return ( a > b ? a : b );
}

// *************************************************************************
inline double vtkPolarAxesActor::FFix( double value )
{
  int ivalue = static_cast<int>( value );
  return ivalue;
}

// *************************************************************************
inline double vtkPolarAxesActor::FSign( double value, double sign )
{
  value = fabs( value );
  if ( sign < 0.)
    {
    value *= -1.;
    }
  return value;
}

// *******************************************************************
void vtkPolarAxesActor::BuildPolarAxisTicks( double origin )
{
  // Find the integral points.
  double range = this->MaximumRadius;
  double pow10 = log10( range );

  // Build in numerical tolerance
  if ( pow10 != 0.)
    {
    double eps = 10.0e-10;
    pow10 = this->FSign( ( fabs( pow10 ) + eps ), pow10 );
    }

  // FFix will move in the wrong direction if pow10 is negative.
  if ( pow10 < 0.)
    {
    pow10 = pow10 - 1.;
    }

  double fxt = pow( 10., this->FFix( pow10 ) );

  // Find the number of integral points in the interval.
  double fnt = range / fxt;
  fnt  = this->FFix( fnt );
  double frac = fnt;
  int numTicks = frac <= 0.5 ? static_cast<int>( this->FFix( fnt ) ) : static_cast<int>( this->FFix( fnt ) + 1 );

  double div = 1.;
  if ( numTicks < 5 )
    {
    div = 2.;
    }
  if ( numTicks <= 2 )
    {
    div = 5.;
    }

  // If there aren't enough major tick points in this decade, use the next
  // decade.
  double major = fxt;
  if ( div != 1.)
    {
    major /= div;
    }

  // Figure out the first major tick locations, relative to the
  // start of the axis.
  double sortedRange[2];
  vtkAxisActor* axis = this->RadialAxes[0];
  double *inRange = axis->GetRange();
  sortedRange[0] = inRange[0] < inRange[1] ? inRange[0] : inRange[1];
  sortedRange[1] = inRange[0] > inRange[1] ? inRange[0] : inRange[1];

  double majorStart;
  if ( sortedRange[0] <= 0.)
    {
    majorStart = major * ( this->FFix( sortedRange[0]*( 1./major ) ) + 0. );
    }
  else
    {
    majorStart = major * ( this->FFix( sortedRange[0]*( 1./major ) ) + 1. );
    }

  axis->SetMajorRangeStart( majorStart );
  axis->SetDeltaRangeMajor( major );

  double t = ( majorStart - sortedRange[0] ) / range;
  majorStart = t * range  + origin;

  // Set major start and deltas
  axis->SetMajorStart( VTK_AXIS_TYPE_X, majorStart );
  axis->SetDeltaMajor( VTK_AXIS_TYPE_X, major );
}

// ****************************************************************
void vtkPolarAxesActor::BuildPolarAxisLabels(  double zeroThreshold )
{
  // Calculate number of labels needed and create array for them
  vtkAxisActor* axis = this->RadialAxes[0];
  double deltaMajor = axis->GetDeltaMajor( VTK_AXIS_TYPE_X );
  double val = axis->GetMajorStart( VTK_AXIS_TYPE_X );
  const double *p2  = axis->GetPoint2Coordinate()->GetValue();
  double lastVal = p2[0];
  vtkIdType nLabels = 0;
  while ( val <= lastVal && nLabels < VTK_MAX_LABELS )
    {
    ++ nLabels;
    val += deltaMajor;
    }
  vtkStringArray *labels = vtkStringArray::New();
  labels->SetNumberOfValues( nLabels );

  // Now create labels
  val = axis->GetMajorRangeStart();
  deltaMajor = axis->GetDeltaRangeMajor();
  const char *format = this->RadialLabelFormat;
  char label[64];
  for ( int  i = 0; i < nLabels; ++ i )
    {
    if ( fabs( val ) < zeroThreshold && this->MaximumRadius > 1 )
      {
      // We just happened to fall at something near zero and the range is
      // large, so set it to zero to avoid ugliness.
      val = 0.;
      }
    sprintf( label, format, val );

    if ( fabs( val ) < zeroThreshold )
      {
      //
      // Ensure that -0.0 is never a label
      // The maximum number of digits that we allow past the decimal is 5.
      //
      if ( strcmp( label, "-0" ) == 0 )
        {
        sprintf( label, "0" );
        }
      else if ( strcmp( label, "-0.0" ) == 0 )
        {
        sprintf( label, "0.0" );
        }
      else if ( strcmp( label, "-0.00" ) == 0 )
        {
        sprintf( label, "0.00" );
        }
      else if ( strcmp( label, "-0.000" ) == 0 )
        {
        sprintf( label, "0.000" );
        }
      else if ( strcmp( label, "-0.0000" ) == 0 )
        {
        sprintf( label, "0.0000" );
        }
      else if ( strcmp( label, "-0.00000" ) == 0 )
        {
        sprintf( label, "0.00000" );
        }
      }
    labels->SetValue( i, label );
    val += deltaMajor;
    }

  // Store labels
  axis->SetLabels( labels );

  // Clean up
  labels->Delete();

  // Update axis label followers
  vtkAxisFollower** labelActors = axis->GetLabelActors();
  for( int i = 0; i < nLabels; ++i )
    {
    labelActors[i]->SetAxis( axis );
    labelActors[i]->SetScreenOffset( this->LabelScreenOffset );
    }
}

// ****************************************************************************
void vtkPolarAxesActor::SetLabelScaling( bool autoscale, int upow )
{
  if ( autoscale != this->AutoLabelScaling || upow != this->UserRadialPow )
    {
    this->AutoLabelScaling = autoscale;
    this->UserRadialPow = upow;
    this->Modified();
    }
}

// ****************************************************************
void vtkPolarAxesActor::AutoScale( vtkViewport *viewport )
{
  // Current implementation only for perspective projections.
  this->AutoScale( viewport, this->RadialAxes );
}

// ****************************************************************
void vtkPolarAxesActor::AutoScale( vtkViewport *viewport, 
                                   vtkAxisActor** axis )
{
  double newTitleScale = this->TitleScale;

  // Loop over radial axes
  for ( int i = 0; i < this->NumberOfRadialAxes; ++ i )
    {
    // Scale title
    newTitleScale = this->AutoScale( viewport, 
                                     this->ScreenSize,
                                     axis[i]->GetTitleActor()->GetPosition() );

    axis[i]->SetTitleScale( newTitleScale );

    // Scale labels
    vtkAxisFollower** labelActors = axis[i]->GetLabelActors();

    for( int j = 0; j < axis[i]->GetNumberOfLabelsBuilt(); ++ j )
      {
      double newLabelScale = this->AutoScale( viewport, 
                                              this->ScreenSize,
                                              labelActors[j]->GetPosition() );

      labelActors[j]->SetScale( newLabelScale );
      }
    }
}

// ****************************************************************
double vtkPolarAxesActor::AutoScale( vtkViewport *viewport, 
                                     double screenSize,
                                     double position[3] )
{
  double factor = 1;
  if ( viewport->GetSize()[1] > 0 )
    {
    factor = 2.0 * screenSize
      * tan( vtkMath::RadiansFromDegrees( this->Camera->GetViewAngle()/2.0 ) )
      / viewport->GetSize()[1];
    }

    double dist = sqrt(
          vtkMath::Distance2BetweenPoints( position,
                                          this->Camera->GetPosition() ));
    double newScale = factor * dist;

    return newScale;
}

// ****************************************************************************
void vtkPolarAxesActor::SetRadialAxesProperty( vtkProperty *prop )
{
  this->RadialAxesProperty->DeepCopy( prop );
  this->Modified();
}

// ****************************************************************************
vtkProperty* vtkPolarAxesActor::GetRadialAxesProperty()
{
  return this->RadialAxesProperty;
}

// ****************************************************************************
void vtkPolarAxesActor::SetPolarArcsProperty( vtkProperty *prop )
{
  this->PolarArcsActor->SetProperty(prop);
  this->Modified();
}

// ****************************************************************************
vtkProperty* vtkPolarAxesActor::GetPolarArcsProperty()
{
  return this->PolarArcsActor->GetProperty();
}
