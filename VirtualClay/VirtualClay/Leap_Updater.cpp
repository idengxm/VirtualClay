#include "stdafx.h"
#include "Leap_Updater.h"

namespace mb = mudbox;

Leap_Updater::Leap_Updater(ID_List *idl,Leap_Hand *l,Leap_Hand *r)
  :frameEvent(this)
{
  //To resize the image, use Image.GenerateUpscaledImage(*targetImg,factor);
  mblog("Creating texture\n");
  idList = idl;
  hand_l = l;
  hand_r = r;
  mblog("Creating tool\n");
  tool = new Leap_Tool(hand_l);
  mblog("Created tool\n");
  tool->SetStamp(RESOURCESDIR+"stamp1.png");
  meshOp = new MeshOps();
  //meshOp_R = new MeshOps();

  //mblog("Listing Nodes\n");
  //for(mb::Node *nodes = mb::Node::First() ; nodes ; nodes = nodes->Next()) {
  //  mblog("Node: "+nodes->Name()+" "+QString::number(nodes->ID())+"\n");
  //}
  //mblog("Listed Nodes\n");
  meshOp->ChangeCamera(new cameraWrapper(idList->getCam(LR(0))));
  //meshOp_R->ChangeCamera(new cameraWrapper(idList->getCam(LR(1))));
  leapReader = new Leap_Reader();
  leapReader->SetScale(mb::Vector(1,1,1));
  frameEvent.Connect(mb::Kernel()->ViewPort()->FrameEvent);
  menuFilter = new Leap_HUD();
  mb::Kernel()->ViewPort()->AddFilter(menuFilter);
  menuFilter->SetVisible(false);
  brushSize = 20;
  inMenu_L = false;
  inMenu_R = false;
  menuLeft = false;
  menuDown = false;
  menuRight = false;
  menuUp = false;
  reqIntersectionForSelection = false;
  thumbGrabModeToggle = false;
  stickyMovement = false;
  selectWithBrushSize = true;
  pinchGrab = true;
  toolStamp = true;
  brushSizeMenuToggle = false;
  SceneNavigationToggle = true;
  pivotHandsOnMesh = true;
  moveObjectMode = false;
  thumbMoveStrength = 10.0f; //Strength and distance for movement intersecting thumb
  savedHandPivotPoint = mb::Vector(0,0,0);

}

mb::Vector Leap_Updater::fitToCameraSpace() {
  mb::Vector camPos = viewCam->getPosision();
  mb::Vector pO = mb::Vector(0,150,300);
  mb::Vector heightOffset = mb::Vector(0,150,0);
  mb::Vector camForward = viewCam->getForward();
  int forwardfactor = (int)(pO.Length());
  mb::Vector centrePoint = camPos - (camForward*forwardfactor) - heightOffset;
  return centrePoint;
}

void Leap_Updater::rotateCamera(mb::Vector r) {
    mb::Vector camPos = viewCam->getTNode()->Position();
    mb::Vector aimPoint = viewCam->getCamera()->Aim();
    viewCam->setPosition(RotateVectorAroundPivot(camPos,aimPoint,r));
    viewCam->getCamera()->SetTarget(aimPoint);
}

void Leap_Updater::ScreenTap(LR lr) {
  meshOp->ChangeCamera(viewCam);
  mb::Vector indexPos;
  if(lr == l) {
    indexPos = hand_l->GetFingerPos(INDEX,TIP);
    hand_l->SetVisi(false);
  } else {
    indexPos = hand_r->GetFingerPos(INDEX,TIP);
    hand_r->SetVisi(false);
  }

  mb::Vector projPos = viewCam->getCamera()->Project(indexPos);
  projPos = projPos * mb::Vector(1,-1,1);
  mblog(VectorToQString(indexPos)+"Pos"+VectorToQStringLine(projPos));
  meshOp->SelectObject(viewCam,projPos);
  //meshOp_R->SelectObject(viewCam,projPos);
  if(lr == l) {
    hand_l->SetVisi(true);
  } else {
    hand_r->SetVisi(true);
  }
}

mb::Vector Leap_Updater::GetRelativeScreenSpaceFromWorldPos(mb::Vector wPos) {
  mb::Camera *activeCam = mb::Kernel()->Scene()->ActiveCamera();
  return activeCam->Project(wPos);
}

bool Leap_Updater::selectMeshPinch() {
  meshOp->ChangeCamera(viewCam);
  mb::Vector indexPos = hand_l->GetFingerPos(INDEX);
  mb::Vector thumbPos = hand_l->GetFingerPos(THUMB);
  if(indexPos.DistanceFrom(thumbPos) < 10) {
    mb::Vector thumbProj = viewCam->getCamera()->Project(thumbPos);
    mb::Vector indexProj = viewCam->getCamera()->Project(indexPos);
    mb::Vector midPoint = (thumbProj+indexProj)*0.5f;
    return meshOp->SelectFaces(l,midPoint,0.0f,0.0f);
  } else {
    mblog("Fingers too far apart");
    return false;
  }
}

bool Leap_Updater::selectMesh(LR lOrR) {
  int handCamID = idList->getCam(lOrR);
  cameraWrapper *handCam = new cameraWrapper(handCamID);

  //handCam->MoveForward(50);
  bool b =false;
  float avgSize = 5;
  meshOp->ChangeCamera(handCam);
  mb::Vector zeroVector = mb::Vector(0.0f,0.0f,0.0f);
  
  if(lOrR == l) {
    hand_l->SetVisi(false);
    hand_l->SetPos(zeroVector);
      avgSize = hand_l->AvgDistFromThumb();
  } else {
    hand_r->SetVisi(false);
    hand_r->SetPos(zeroVector);
    avgSize = hand_r->AvgDistFromThumb();
  }
  if(selectWithBrushSize) 
    avgSize = brushSize;
  mb::Kernel()->Scene()->SetActiveCamera(handCam->getCamera());
  mb::Kernel()->ViewPort()->Redraw();  
  mblog("Brush Size = "+QString::number(avgSize));
  b = meshOp->SelectFaces(lOrR,avgSize,10);
  hand_l->SetVisi(true);
  mb::Kernel()->Scene()->SetActiveCamera(viewCam->getCamera());
  mb::Kernel()->ViewPort()->Redraw();
  return b;
}

void Leap_Updater::MoveMesh(LR lOrR) {
  mb::Vector currentHandPos;
  mb::Vector distanceDiff;
  if(lOrR == l) {
  currentHandPos = hand_l->GetPos();
    distanceDiff = currentHandPos - lastFrameHandPos_L;
    lastFrameHandPos_L = currentHandPos;
  } else {
    currentHandPos = hand_r->GetPos();
    distanceDiff = currentHandPos - lastFrameHandPos_R;
    lastFrameHandPos_R = currentHandPos;
  }
  if(distanceDiff.x > 10 || distanceDiff.y > 10 || distanceDiff.z > 10) {
    distanceDiff = mb::Vector(0,0,0);
  }
  //mblog("Moving Mesh currentHandPos: "+VectorToQString(currentHandPos)+
  //  "lastFrameHandPos_L: "+VectorToQString(lastFrameHandPos_L)+
  //  "DistanceDiff: "+VectorToQStringLine(distanceDiff));
  meshOp->MoveVertices(lOrR,distanceDiff);
}

int Leap_Updater::countIntersectingFingers(LR lOrR) {
  int counter = 0;
  if(lOrR == l) {
    for(int i = 0; i < 5 ; i++) {
      if(meshOp->CheckIntersection(hand_l->GetFingerBoundingBox(fingerEnum(i)))) {
        counter++;
      }
    }
  } else {
    for(int i = 0; i < 5 ; i++) {
      if(meshOp->CheckIntersection(hand_r->GetFingerBoundingBox(fingerEnum(i)))) {
        counter++;
      }
    }
  }
  return counter;
}

int Leap_Updater::countTouchingFingers(LR lOrR) {
  int counter = 0;
  if(lOrR == l) {
    for(int i = 0; i < 5 ; i++) {
      if(meshOp->CheckTouching(hand_l->GetFingerBoundingBox(fingerEnum(i)))) {
        counter++;
      }
    }
  } else {
    for(int i = 0; i < 5 ; i++) {
      if(meshOp->CheckTouching(hand_r->GetFingerBoundingBox(fingerEnum(i)))) {
        counter++;
      }
    }
  }
  return counter;
}

bool Leap_Updater::ThumbSelect() {
  mb::Vector thumbPos = hand_l->GetFingerPos(THUMB,TIP);
  mb::Vector thumbProj = viewCam->getCamera()->Project(thumbPos);
  hand_l->SetVisi(false);
  mblog("Thumb Proj Pos = "+VectorToQStringLine(thumbProj));
  mblog("Thumb Proj Pos Pixels = "+VectorToQStringLine(ScreenSpaceToPixels(thumbProj)));
  //float save = thumbProj.x;
  //thumbProj.x = thumbProj.y;
  //thumbProj.y = save;
  meshOp->ChangeCamera(viewCam);
  if(!facesAreSelected_L&& meshOp->SelectFaces(l,ScreenSpaceToPixels(thumbProj),10.0f,5)) {
    facesAreSelected_L= true;
  } else {
    mb::Vector currentThumbPos = hand_l->GetFingerPos(THUMB,TIP);
    mb::Vector distanceDiff = currentThumbPos - lastFrameThumbPos;
    if(distanceDiff.x > 10 || distanceDiff.y > 10 || distanceDiff.z > 10) {
      distanceDiff = mb::Vector(0,0,0);
    }
    //mblog("Moving Mesh currentHandPos: "+VectorToQString(currentHandPos)+
    //  "lastFrameHandPos_L: "+VectorToQString(lastFrameHandPos_L)+
    //  "DistanceDiff: "+VectorToQStringLine(distanceDiff));
    meshOp->MoveVertices(l,distanceDiff);
    lastFrameThumbPos = currentThumbPos;
    mbstatus("Moving faces");
    mblog("Moving Faces");
  }
  return true;
}

bool Leap_Updater::ThumbSmoothMove() {
  mb::Vector thumbPos = hand_l->GetFingerPos(THUMB,TIP);
  mb::Vector thumbProj = viewCam->getCamera()->Project(thumbPos);
  thumbProj = thumbProj * mb::Vector(1,-1,1);
  hand_l->SetVisi(false);
  mblog("Thumb Proj Pos = "+VectorToQStringLine(thumbProj));
  mblog("Thumb Proj Pos Pixels = "+VectorToQStringLine(ScreenSpaceToPixels(thumbProj)));
  meshOp->ChangeCamera(viewCam);
  if(meshOp->SelectFaces(l,ScreenSpaceToPixels(thumbProj),10.0f,10)) {
    mb::Vector dirNorm = leapReader->getMotionDirection(THUMB,l);
    mblog("Normalised Direction = "+VectorToQStringLine(dirNorm));
    mb::Vector dist = dirNorm*thumbMoveStrength;
    meshOp->MoveVertices(l,dist);

    hand_l->SetVisi(true);
    return true;
  }
  hand_l->SetVisi(true);
  return false;
}

__inline void Leap_Updater::SetHandAndFingerPositions() {
  //TODO:
  // Do I need to actually rotate the fingers?? If so, by what metric?
  // hand_l->SetFingerRot(fingerEnum(i),leapReader->getFingerDirection_L(fingerEnum(i)));
  QTime *overall = new QTime();
  overall->start();
  QTime *t = new QTime();
  t->start();
  mb::Vector camRot = viewCam->getTNode()->Rotation();
  hand_l->SetVisi(leapReader->isVisible(l));
  hand_r->SetVisi(leapReader->isVisible(r));
  //mblog(" Set Visi Time: "+QString::number(t->elapsed())+"\n");
  t->restart();
  int leftCamID = idList->getCam(l);
  int rightCamID = idList->getCam(r);
  //mblog(" get Cam IDs Time: "+QString::number(t->elapsed())+"\n");
  
  // Set hand position and orientation
  mb::Vector tmp = cameraPivot + leapReader->getPosition_L();
  hand_l->SetPos(tmp);
  tmp = cameraPivot + leapReader->getPosition_R();
  hand_r->SetPos(tmp);
  
  //mblog(" Step 1 Time: "+QString::number(t->elapsed())+"\n");
  t->restart();
  //Rotate the hands XZY
  mb::Vector rotation = (mb::Vector(1,1,-1)*camRot) + leapReader->getDirection_L();
  mb::Matrix rX = createRotateXMatrix(rotation.x);
  mb::Matrix rY = createRotateYMatrix(rotation.y);
  mb::Matrix rZ = createRotateZMatrix(rotation.z);
  mb::Matrix rotationMatrix = rX*rZ*rY;
  hand_l->GetTNode()->SetRotation(rotationMatrix);

  mb::Vector rotation_r = (mb::Vector(-1,1,-1)*camRot) + leapReader->getDirection_R();
  mb::Matrix rX_r = createRotateXMatrix(rotation_r.x);
  mb::Matrix rY_r = createRotateYMatrix(rotation_r.y);
  mb::Matrix rZ_r = createRotateZMatrix(rotation_r.z);
  mb::Matrix rotationMatrix_r = rZ_r*rX_r*rY_r;
  hand_r->GetTNode()->SetRotation(rotationMatrix_r);
  
  //mblog(" Rotation Matrix Calc Time: "+QString::number(t->elapsed())+"\n");
  t->restart();
  hand_l->RotateAroundPivot(-1*camRot,cameraPivot);
  hand_r->RotateAroundPivot(-1*camRot,cameraPivot);

  //Setting Cameras to follow Hand;
  cameraWrapper *leftHand = new cameraWrapper(leftCamID);
  leftHand->getTNode()->SetRotation(hand_l->GetRot()+mb::Vector(80,0,0));
  leftHand->getTNode()->SetPosition(hand_l->GetPos());
  leftHand->MoveForward(-10.0f);
  cameraWrapper *rightHand = new cameraWrapper(rightCamID);
  rightHand->getTNode()->SetRotation(hand_r->GetRot()+mb::Vector(80,0,0));
  rightHand->getTNode()->SetPosition(hand_r->GetPos());
  rightHand->MoveForward(-20.0f);
  //bool leftColl = false;
  //mblog("Finger IntersectCount = "+QString::number(countIntersectingFingers(l))+"\n");

  //For Coll detection and replacement:
  mb::Vector fingerPos_L = mb::Vector(0,0,0);
  mb::Vector fingerPos_R = mb::Vector(0,0,0);
  //mblog(" Hand Set Time Calc Time: "+QString::number(t->elapsed())+"\n");
  t->restart();
  for(int i = 0 ; i < 5 ; i++) {
    for(int j = 0 ; j < 4 ; j++) {
      fingerPos_L = hand_l->GetFingerPos(fingerEnum(i),jointEnum(j));
      fingerPos_R = hand_r->GetFingerPos(fingerEnum(i),jointEnum(j));
      tmp = cameraPivot + leapReader->getFingerPosition(fingerEnum(i),l).at(j);
      hand_l->SetFingerPos(jointEnum(j),fingerEnum(i),tmp);
      hand_l->RotateAroundPivot(jointEnum(j),fingerEnum(i),-1*camRot,cameraPivot);

      tmp = cameraPivot + leapReader->getFingerPosition(fingerEnum(i),r).at(j);
      hand_r->SetFingerPos(jointEnum(j),fingerEnum(i),tmp);
      hand_r->RotateAroundPivot(jointEnum(j),fingerEnum(i),-1*camRot,cameraPivot);

      //if(collisionToggle) {
      //  if(meshOp->CheckIntersection(hand_l->GetFingerBoundingBox(fingerEnum(i),jointEnum(j)))) {
      //    if(j == 0) {
      //      hand_l->SetFingerPos(jointEnum(j),fingerEnum(i),fingerPos_L);
      //    }
      //    //mblog("L Hand collision!\n");
      //  }
      //  if(meshOp->CheckIntersection(hand_r->GetFingerBoundingBox(fingerEnum(i),jointEnum(j)))) {
      //    if(j == 0) {
      //      hand_r->SetFingerPos(jointEnum(j),fingerEnum(i),fingerPos_R);
      //    }
      //    //mblog("R Hand collision!\n");
      //  }
      //}
      // }
      //mblog("Updating Bone Pos :"+QString::number(j)+"\n");
      //hand_r->UpdateBone(boneEnum(j),fingerEnum(i),leapReader->getBoneOrients(fingerEnum(i),r).at(j));
      //hand_l->UpdateBone(boneEnum(j),fingerEnum(i),leapReader->getBoneOrients(fingerEnum(i),l).at(j));
      //mblog("Updated Bone Pos\n");
      }
    }
  hand_l->SetAllFingerJointRots();
  hand_r->SetAllFingerJointRots();
  //mblog(" Loop for Fingers: "+QString::number(t->elapsed())+"\n");
  t->restart();
  if(leapReader->isTool) {
    tool->SetVisi(true);
    std::vector<mb::Vector> toolLocs = leapReader->GetToolPositions();
    tmp = cameraPivot+toolLocs.at(0);
    tool->SetPos(0,tmp);
    tmp = cameraPivot+toolLocs.at(1);
    tool->SetPos(1,tmp);
    tmp = (mb::Vector(-1,1,-1)*camRot)+leapReader->GetToolDirection();
    tool->SetRot( tmp);
    tool->SetRot(0,GetAimRotation(tool->GetPos(0),mb::Vector(0,0,0)));
    tmp = (-1*camRot)/2;
    tool->RotateAroundPivot(tmp,cameraPivot);
    tool->RotateAroundPivot(tmp,cameraPivot);
  } else {
    tool->SetVisi(false);
  }
  //mblog(" Tools Time: "+QString::number(t->elapsed())+"\n");
  //mblog("  Overall time"+QString::number(overall->elapsed())+"\n");
}

void Leap_Updater::CameraRotate(LR lOrR) {
  const float deadzone = 20.0f;
  mb::Vector handRot;
  if(lOrR == r) {
    handRot = leapReader->getDirection_R()+mb::Vector(15,0,0);
  } else {
    handRot = leapReader->getDirection_L()+mb::Vector(15,0,0);
  }
  mbstatus("HandRot: "+VectorToQString(handRot));
  if(abs(handRot.x) > deadzone || abs(handRot.y) > deadzone || abs(handRot.z) > deadzone) {
    if(abs(handRot.x) > abs(handRot.y) && abs(handRot.x) > abs(handRot.z)) {
      mblog("Pitch\n");
      mbhud("Rotate Pitch");
      if(handRot.x > 0)
        rotateCamera(mb::Vector(0.5,0,0));
      else
        rotateCamera(mb::Vector(-0.5,0,0));
    }
    if(abs(handRot.y) > abs(handRot.x) && abs(handRot.y) > abs(handRot.z)) {
      mblog("Roll\n");
      mbhud("Rotate Roll");
      if(handRot.y > 0)
        rotateCamera(mb::Vector(0,0.5,0));
      else
        rotateCamera(mb::Vector(0,-0.5,0));
    }
    if(abs(handRot.z) > abs(handRot.x) && abs(handRot.z) > abs(handRot.y)) {
      mblog("Yaw\n");
      mbhud("Rotate Yaw");
      if(handRot.z > 0)
        rotateCamera(mb::Vector(0,0,0.5));
      else
        rotateCamera(mb::Vector(0,0,-0.5));
    }

  }
  //viewCam->getTNode()->SetPosition(sceneRotate);
}

void Leap_Updater::CameraZoom(LR lOrR) {
  const float deadzone = 20.0f;
  mb::Vector handRot;
  if(lOrR == r) {
    handRot = leapReader->getPosition_R()+mb::Vector(30,0,0);
  } else {
    handRot = leapReader->getPosition_L()+mb::Vector(30,0,0);
  }
  mbstatus("HandRot: "+VectorToQString(handRot));
  if(abs(handRot.z) > deadzone) {
      if(handRot.z > 0) {
        mblog("Forward\n");
        viewCam->getCamera()->MoveForward(mb::Vector(10,0,0));
      } else {
        mbhud("Backward\n");
        viewCam->getCamera()->MoveBackward(mb::Vector(10,0,0));
      }
    }
  //viewCam->getTNode()->SetPosition(sceneRotate);
}

void Leap_Updater::MenuSettings_R() {
  //https://helpx.adobe.com/illustrator/using/drawing-simple-lines-shapes.html
  mb::Vector PosDifference = menuStartSpace - GetRelativeScreenSpaceFromWorldPos(hand_r->GetFingerPos(INDEX));
  menuFilter->menuChoice = 5;
  if(PosDifference.y > menuDeadZone) {
    menuFilter->menuChoice = 8;
    if(PosDifference.y > menuActivateZone)
      menuDown = true;
  } else if(PosDifference.y < -menuDeadZone) {
    menuFilter->menuChoice = 6;
    if(PosDifference.y < -menuActivateZone)
      menuUp = true;
  }
  if(abs(PosDifference.x) > abs(PosDifference.y)) {
    if(PosDifference.x > menuDeadZone) {
      menuFilter->menuChoice = 9;
      if(PosDifference.x > menuActivateZone) {
        menuLeft = true;
      }
    } else if(PosDifference.x < -menuDeadZone) {
      menuFilter->menuChoice = 7;
      if(PosDifference.x < -menuActivateZone) {
        menuRight = true;
      }
    }
    if(leapReader->isCircleCCW_R) {
      mbstatus("Got Anti-Clockwise Circle");
      menuFilter->SetVisible(false);
      inMenu_R = false;
    }
  }
  if(menuUp) {
    pivotHandsOnMesh =! pivotHandsOnMesh;
    /*thumbGrabModeToggle = !thumbGrabModeToggle;
    if(thumbGrabModeToggle)
      mbhud("Thumb Grab Mode: On");
    else
      mbhud("Thumb Grab Mode: Off");
    */
    if(pivotHandsOnMesh)
      mbhud("Pivot Hands On Mesh");
    else
      mbhud("Pivot Hands On Camera");
    menuFilter->SetVisible(false);
    inMenu_R = false;
    menuUp = false;
  } else if(menuRight) {
    //Collision Toggle
    reqIntersectionForSelection = !reqIntersectionForSelection;
    selectWithBrushSize = !reqIntersectionForSelection;
    if(reqIntersectionForSelection)
      mbhud("Collision Mode: On");
    else
      mbhud("Collision Mode: Off");
    menuFilter->SetVisible(false);
    inMenu_R = false;
    menuRight = false;
  } else if(menuDown) {
    //Move Obj mode
//    pinchGrab = !pinchGrab;
    moveObjectMode = !moveObjectMode;
    //if(pinchGrab)
    //  mbhud("Pinch Grab On");
    //else
    //  mbhud("PinchGrab Off");
    if(moveObjectMode)
      mbhud("MoveObject Mode On");
    else
      mbhud("MoveObject Mode Off");
    menuFilter->SetVisible(false);
    inMenu_R = false;
    menuDown = false;
  } else if(menuLeft) {
    //Back
    menuFilter->SetVisible(false);
    inMenu_R = false;
    menuLeft = false;
  }
}

void Leap_Updater::MenuSettings_L() {
  mb::Vector PosDifference = menuStartSpace - GetRelativeScreenSpaceFromWorldPos(hand_l->GetFingerPos(INDEX));
  menuFilter->menuChoice = 0;
  if(PosDifference.y > menuDeadZone) {
    menuFilter->menuChoice = 3;
    if(PosDifference.y > menuActivateZone)
      menuDown = true;
  } else if(PosDifference.y < -menuDeadZone) {
    menuFilter->menuChoice = 1;
    if(PosDifference.y < -menuActivateZone)
      menuUp = true;
  }
  if(abs(PosDifference.x) > abs(PosDifference.y)) {
    if(PosDifference.x > menuDeadZone) {
      menuFilter->menuChoice = 4;
      if(PosDifference.x > menuActivateZone) {
        menuLeft = true;
      }
    } else if(PosDifference.x < -menuDeadZone) {
      menuFilter->menuChoice = 2;
      if(PosDifference.x < -menuActivateZone) {
        menuRight = true;
      }
    }
    if(leapReader->isCircleCCW_L) {
      mbstatus("Got Anti-Clockwise Circle");
      menuFilter->SetVisible(false);
      inMenu_L = false;
    }
  }

  if(menuUp) {
    brushSizeMenuToggle = true;
    brushSizeStartFingerStartPos = GetRelativeScreenSpaceFromWorldPos(hand_l->GetFingerPos(INDEX));
    menuFilter->menuChoice = 10;
    inMenu_L = false;
    menuUp = false;
  } else if(menuRight) {
    toolStamp = !toolStamp;
    if(toolStamp)
      mbhud("Tool Stamp Mode: On");
    else
      mbhud("Tool Stamp Mode: Off");
    menuFilter->SetVisible(false);
    inMenu_L = false;
    menuRight = false;
  } else if(menuDown) {
    SceneNavigationToggle = !SceneNavigationToggle;
    if(SceneNavigationToggle)
      mbhud("SceneNavigation On");
    else
      mbhud("SceneNavigation Off");
    menuFilter->SetVisible(false);
    inMenu_L = false;
    menuDown = false;
  } else if(menuLeft) {
    menuFilter->SetVisible(false);
    inMenu_L = false;
    menuLeft = false;
  }
}

void Leap_Updater::BrushSize() {
  if(leapReader->isScreenTap_L || leapReader->isScreenTap_R) {
    mblog("got tap\n");
    brushSizeMenuToggle = false;
    menuFilter->SetVisible(false);
    return;
  }
  mb::Vector PosDifference = menuStartSpace - GetRelativeScreenSpaceFromWorldPos(hand_l->GetFingerPos(INDEX));
  //include a deadzone
  const float scalefactor = 0.5;
  if(PosDifference.y > 0.1f) {
    brushSize = MAX(brushSize - (PosDifference.y-0.05)*scalefactor,0);
  } else if(PosDifference.y < 0.1f) {
    brushSize = MAX(brushSize - (PosDifference.y+0.05)*scalefactor,0);
  } else {
    return;
  }
  mb::Kernel()->Interface()->HUDMessageHide();
  mbhud("Brush size = "+QString::number(brushSize));
  float relativeSize = brushSize/mb::Kernel()->ViewPort()->Height();
  mblog("Relative size = "+QString::number(relativeSize)+"\n");
  menuFilter->SetSize(relativeSize);
}

void Leap_Updater::Extrusion(LR LorR) {
  if(LorR == l) {
    mblog("Finger IntersectCount = "+QString::number(countIntersectingFingers(l))+"\n");
    if(!facesAreSelected_L){
      if(!reqIntersectionForSelection || (countIntersectingFingers(l) > 3)) {
        if(selectMesh(l) ) {
          facesAreSelected_L= true;
          firstmoveswitch = true;
          mblog("Succesfully Selected\n");
        } else {
          mbhud("Failed To Select L_Hand, Have you selected the Mesh?");
          mblog("Failed to select\n");
        }
      }
      mbstatus("Grabbing");
    } else {
      if(firstmoveswitch) {
        lastFrameHandPos_L = hand_l->GetPos();
        firstmoveswitch = false;
      } else {
        mbstatus("MovingMesh");
        mblog("Moving Mesh\n");
        MoveMesh(l);
      }
    }
  } else {
    if(!facesAreSelected_R) {
      if(!reqIntersectionForSelection || (countIntersectingFingers(r) > 3)) {
        if(selectMesh(r) ) {
          facesAreSelected_R = true;
          firstmoveswitch_R = true;
          mblog("Succesfully Selected\n");
        } else {
          mbhud("Failed To Select with R_Hand, Have you selected the Mesh?");
          mblog("Failed to select\n");
        }
      }
      mbstatus("Grabbing");
    } else {
      if(firstmoveswitch_R) {
        lastFrameHandPos_R = hand_r->GetPos();
        firstmoveswitch_R = false;
      } else {
        mbstatus("MovingMesh");
        mblog("Moving Mesh\n");
        MoveMesh(r);
      }
    }

  }
}

__inline void Leap_Updater::checkNavigationGestures() {
  if(SceneNavigationToggle) {
    if(leapReader->isVisible(l)) {
      if(leapReader->CheckRotateHandGesture(l)) {
        CameraRotate(l);
      } else if(leapReader->CheckScaleHandGesture(l)) {
        CameraZoom(l);
      }
    }
    if(leapReader->isVisible(r)) {
      if(leapReader->CheckRotateHandGesture(r)) {
        CameraRotate(r);
      } else if(leapReader->CheckScaleHandGesture(r)) {
        CameraZoom(r);
      }
    }
  }
}

__inline void Leap_Updater::checkMenuGesture() {
    //If Circle Clockwise
  if(leapReader->isCircleCW_R) {
    if(leapReader->CheckFingerExtensions(r,false,true,true,false,false)) {
      menuStartSpace = GetRelativeScreenSpaceFromWorldPos(hand_r->GetFingerPos(INDEX));
      menuFilter->SetCentre(menuStartSpace);
      menuFilter->SetVisible(true);
      inMenu_R = true;
    }
  }
  if(leapReader->isCircleCW_L) {
    if(leapReader->CheckFingerExtensions(l,false,true,true,false,false)) {
      menuStartSpace = GetRelativeScreenSpaceFromWorldPos(hand_l->GetFingerPos(INDEX));
      menuFilter->SetCentre(menuStartSpace);
      menuFilter->SetVisible(true);
      inMenu_L = true;
    }
  }
}

__inline void Leap_Updater::checkScreenTapGesture() {
  //If they tapped the screen
  if(leapReader->CheckFingerExtensions(l,false,true,false,false,false)) {
    if(leapReader->isScreenTap_L) {
      mblog("got tap\n");
      ScreenTap(l);
    }
  } else if(leapReader->CheckFingerExtensions(l,false,true,false,false,false)) {
    if(leapReader->isScreenTap_R) {
      mblog("got tap\n");
      ScreenTap(r);
    }
  }
}

__inline void Leap_Updater::checkUndoGesture() {
  //If they Swiped Left
  if(leapReader->isUndo) {
    mb::Kernel()->Interface()->HUDMessageShow("UNDO");
    meshOp->UndoLast();
    //meshOp_R->UndoLast();
  }
}

__inline void Leap_Updater::checkGrabbingGesture() {
  if(thumbGrabModeToggle) {
    if((meshOp->CheckTouching(hand_l->GetFingerBoundingBox(THUMB,TIP)))) {
      if(meshOp->firstUse) {
        meshOp->firstUse = false;
      }
      if(stickyMovement)
        ThumbSelect();
      else
        ThumbSmoothMove();
    } else {
      meshOp->firstUse = true;
      if(facesAreSelected_L){
        meshOp->DeselectAllFaces();
      }
      facesAreSelected_L= false;
    }
  } else {
  //If they are Grabbing and Thumb Toggle is Off
    if(leapReader->isGrabbing_L ) {
      Extrusion(l);
      if(leapReader->isGrabbing_R) {
        mbstatus("Right hand grabbing too\n");
        Extrusion(r);
      }
    } else if(leapReader->isGrabbing_R) {
      Extrusion(r);
    } else {
      if(facesAreSelected_R || facesAreSelected_L){
        meshOp->FindTesselationFaces(l);
        mblog("going to deselect faces");
        //meshOp->DeselectAllFaces();
        //meshOp_R->DeselectAllFaces();
      }
      firstmoveswitch = true;
      facesAreSelected_R = false;
      facesAreSelected_L= false;
    }
    /*
    } else {
      if(facesAreSelected_L){
        meshOp->DeselectAllFaces();
      }
      firstmoveswitch_R = true;
      facesAreSelected_L= false;
    }*/
    lastFrameHandPos_L = hand_l->GetPos();
    lastFrameHandPos_R = hand_r->GetPos();
  }
}

void Leap_Updater::ToolStampMove() {
  int toolCamID = idList->getToolCam();
  cameraWrapper *toolCam= new cameraWrapper(toolCamID);
  toolCam->setPosition(tool->GetPos(1));
  toolCam->setAim(tool->GetPos(0));
  toolCam->MoveForward(-50);
  meshOp->ChangeCamera(viewCam);
  tool->SetVisi(false);
  mblog("ToolStamp\n");
  int dist = 10;
  mb::Vector toolPos = tool->GetPos(0);
  mb::Vector toolProj = viewCam->getCamera()->Project(toolPos);
  mblog("Tool Proj Pos = "+VectorToQStringLine(toolProj));
  toolProj = toolProj * mb::Vector(1,-1,1);
  mblog("Tool Proj Pos Pixels = "+VectorToQStringLine(ScreenSpaceToPixels(toolProj)));
  tool->ResizeStamp(brushSize,brushSize);
  if(meshOp->ToolManip(ScreenSpaceToPixels(toolProj),20.0f,tool)) {
    facesAreSelected_Tool = true;
    mblog("Moving vertices maybe?\n");
    meshOp->MoveVertices(r,dist);
  }
  tool->SetVisi(true);
}

void Leap_Updater::ToolSmoothMove() {
  int toolCamID = idList->getToolCam();
  cameraWrapper *toolCam= new cameraWrapper(toolCamID);
  toolCam->setPosition(tool->GetPos(1));
  toolCam->MoveForward(-50);
  toolCam->setAim(tool->GetPos(0));
  meshOp->ChangeCamera(viewCam);
  tool->SetVisi(false);
  mb::Vector toolPos = tool->GetPos(0);
  mb::Vector toolProj = viewCam->getCamera()->Project(toolPos);
  toolProj = toolProj * mb::Vector(1,-1,1);
  mblog("ToolSmoothMove\n");
  mblog("Tool Proj Pos = "+VectorToQStringLine(toolProj));
  mblog("Tool Proj Pos Pixels = "+VectorToQStringLine(ScreenSpaceToPixels(toolProj)));
  //meshOp->ChangeCamera(viewCam);
  if(meshOp->SelectFaces(r,ScreenSpaceToPixels(toolProj),10.0f,10)) {
    facesAreSelected_Tool = true;
    mb::Vector dirNorm = leapReader->getToolMotionDirection();
    mblog("Normalised Direction = "+VectorToQStringLine(dirNorm));
    mb::Vector dist = dirNorm*thumbMoveStrength;
    meshOp->MoveVertices(r,dist);
  }
  tool->SetVisi(true);
}

__inline void Leap_Updater::checkToolIntersection() {
  if(leapReader->isTool) {
    if(meshOp->CheckIntersection(tool->GetInteractionBox())) {
      tool->SendToServer(1);
      mblog("Sending 1\n");
      if(meshOp->CheckIntersection(tool->GetBoundingBox(0))) {
        tool->SendToServer(2);
        mblog("Sending 2\n");
        mblog("Tool bounding box Pos = "+VectorToQStringLine(tool->GetBoundingBox(0).Center()));
       // mblog("Test Tool bounding box Pos = "+VectorToQStringLine(mb::AxisAlignedBoundingBox(tool->GetPos(0),0.2f).Center()));
        //http://www.sciencedirect.com/science/article/pii/S0010448512002734
        //Set the undo list to iterate on.
        if(meshOp->firstUse) {
          meshOp->firstUse = false;
        }
        if(toolStamp) {
          ToolStampMove();
        } else {
          ToolSmoothMove();
        }
      } else {
        if(facesAreSelected_Tool) {
          meshOp->DeselectAllFaces();
        }
        facesAreSelected_Tool = false;
        //Set the undo list to iterate on.
        meshOp->firstUse = true;
      }
    } else {
        tool->SendToServer(0);
    }
  } else {
    tool->SendToServer(0);
  }
}

void Leap_Updater::MoveSelectedObject(LR lr) {
  mb::Vector moveDist;
  if(lr == l) {
    moveDist = hand_l->GetPos() - lastFrameHandPos_L;
    lastFrameHandPos_L = hand_l->GetPos();
  } else {
    moveDist = hand_r->GetPos() - lastFrameHandPos_R;
    lastFrameHandPos_R = hand_r->GetPos();
  }
  meshOp->MoveObject(moveDist);
}

__inline void Leap_Updater::checkMoveObjGesture() {
  if(leapReader->isGrabbing_L) {
    if(isFirstGrab) {
      lastFrameHandPos_L = hand_l->GetPos();
      isFirstGrab = false;
      meshOp->SelectWholeMesh();
    }
    MoveSelectedObject(l);
  } else if(leapReader->isGrabbing_R) {
    if(isFirstGrab) {
      lastFrameHandPos_R = hand_r->GetPos();
      isFirstGrab = false;
      meshOp->SelectWholeMesh();
    }
    MoveSelectedObject(r);
  } else {
    if(!isFirstGrab) {
      meshOp->StoreLastMoveUndoQueue();
      isFirstGrab = true;
      meshOp->DeselectAllFaces();
    }
  }
}

__inline void Leap_Updater::checkUndoMoveGesture() {
  //If they Swiped Left
  if(leapReader->isUndo) {
    mb::Kernel()->Interface()->HUDMessageShow("UNDO Move");
    meshOp->UndoLastMove();
  }
}

void Leap_Updater::OnEvent(const mb::EventGate &cEvent) {
  QTime *overall = new QTime();
  overall->start();
  QTime *t = new QTime();
  t->start();
  if(cEvent == frameEvent) {
    mblog("getting view cam\n");
    int viewcamID = idList->getViewCam();
    mblog("got view cam\n");
    viewCam = new cameraWrapper(viewcamID);
    mblog("setViewCam\n");
    t->restart();
    if(leapReader->updateAll()) {
      //mblog("Update All Time "+QString::number(t->elapsed())+"\n");
      if((leapReader->ishands || leapReader->isTool) && leapReader->isConnected) {
        //mblog("Getting CameraID\n");
        if(pivotHandsOnMesh) {
          cameraPivot = fitToCameraSpace();
        } else {
          cameraPivot = savedHandPivotPoint;
        }
        t->restart();
        SetHandAndFingerPositions();
        //mblog("Set Hands and Finger Position Times: "+QString::number(t->elapsed())+"\n");
        //If Circle Anti-Clockwise
        if(inMenu_R) {
          MenuSettings_R();
        } else if(inMenu_L) {
          MenuSettings_L();
        } else if(brushSizeMenuToggle) {
          BrushSize();
        } else if(moveObjectMode) {
          //Important to keep this ordering, menus first then other modes, ensures we can exit and
          //enter menus at any time!
          t->restart();
          checkMenuGesture();
          //mblog("Check Menu Gesture Times: "+QString::number(t->elapsed())+"\n");
          t->restart();
          checkNavigationGestures();
          //mblog("Check Nav Gesture Times: "+QString::number(t->elapsed())+"\n");
          t->restart();
          checkScreenTapGesture();
          //mblog("Check Screen Tap Gesture Times: "+QString::number(t->elapsed())+"\n");
          t->restart();
          checkUndoMoveGesture();
          //mblog("Check Undo Gesture Times: "+QString::number(t->elapsed())+"\n");
          t->restart();
          checkMoveObjGesture();
          //mblog("Check Move Obj Gesture Times: "+QString::number(t->elapsed())+"\n");
        } else {
          t->restart();
          checkNavigationGestures();
          //mblog("Check Nav Gesture Times: "+QString::number(t->elapsed())+"\n");
          t->restart();
          checkMenuGesture();
          //mblog("Check Menu Gesture Times: "+QString::number(t->elapsed())+"\n");
          t->restart();
          checkScreenTapGesture();
          //mblog("Check Screen Tap Gesture Times: "+QString::number(t->elapsed())+"\n");
          t->restart();
          checkUndoGesture();
          //mblog("Check Undo Gesture Times: "+QString::number(t->elapsed())+"\n");
          t->restart();
          checkGrabbingGesture();
          //mblog("Check Grabbing Gesture Times: "+QString::number(t->elapsed())+"\n");
          t->restart();
          checkToolIntersection();
          //mblog("Check Tool Intersection Times: "+QString::number(t->elapsed())+"\n");
        }
      }
    }
  }
  //mblog("Full Loop Time: "+QString::number(overall->elapsed())+"\n");
}
