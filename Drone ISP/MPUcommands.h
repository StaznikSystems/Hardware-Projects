/* ================================================================
About:
Commands to retrive data from the drone's onboard MPU for use in 
Avionics

Uses the I2C dev library and MPU libraries by Jeff Rowberg 
https://github.com/jrowberg/i2cdevlib/tree/master


Contents:
- MPUInitialize -> Initializes the MPU and DMP
- getYaw, getPitch, getRoll -> gets the rotation in degrees (euler 
angles)
- getXAccel, getYAccel, getZAccel -> gets the Acceleration without 
gravity with earth reference

Contributers:
Goran Staznik (2025) -> Main Contributer
Jeff Rowberg (2012) -> MPU library (used to acces the
internal DMP due to lack of documentation and to compute 
Quaterions)
 ==================================================================
*/


// ================================================================
// ===               LIBRARIES AND DEPENDANCIES                 ===
// ================================================================

#include <I2Cdev.h>                      //Library dependancy
#include <MPU6050_6Axis_MotionApps20.h>  //Main library for DMP

#if I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE
#include <Wire.h>  //Required if the I2C dev has a wire library comand
#endif



// ================================================================
// ===                 CONTAINERS AND VARIABLES                 ===
// ================================================================

uint8_t fifoBuffer[64];   // FIFO storage buffer
uint8_t MPU_ADDR = 0x68;  //Address MPU, default 0x68, 0x69 when ADO is high

// Orientation Vars
Quaternion q;         // [w, x, y, z]         quaternion container
VectorInt16 aa;       // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;   // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;  // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;  // [x, y, z]            gravity vector
float ypr[3];         // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector


// ================================================================
// ===               INITALIZER FOR THE MPU 6050                ===
// ================================================================
//Initialize the MPU and DMP

uint8_t MPUInitialize(uint8_t ADDR, int XGyroOffset, int YGyroOffset, int ZGyroOffset, int XAccelOffset, int YAccelOffset, int ZAccelOffset) {
  MPU_ADDR = ADDR;                                  //set the global address of the MPU to the defined address
  MPU6050 mpu(MPU_ADDR);                            //Set address of MPU (library Dependancy)
#if (I2CDEV_IMPLEMENTATION == I2CDEV_ARDUINO_WIRE)  //Merges the two wire libraries
  Wire.begin();                                     //Start the wire library
  Wire.setClock(400000);                            // 400kHz I2C clock. Comment this line if having compilation difficulties
#elif (I2CDEV_IMPLEMENTATION == I2CDEV_BUILTIN_FASTWIRE)
  Fastwire::setup(400, true);  //start the fastwire I2C protocol (from library)
#endif

  mpu.initialize();               //initalize MPU through the library
  if (mpu.testConnection()) {                 //see if the MPU works
    uint8_t devStatus = mpu.dmpInitialize();  //holds wether DMP initizlization was sucessfull
    //Custom calibration go here
    mpu.setXGyroOffset(XGyroOffset);    //51
    mpu.setYGyroOffset(YGyroOffset);    //8
    mpu.setZGyroOffset(ZGyroOffset);    //21
    mpu.setXAccelOffset(XAccelOffset);  //1150
    mpu.setYAccelOffset(YAccelOffset);  //-50
    mpu.setZAccelOffset(ZAccelOffset);  //1060

    if (!devStatus) {         //devsatus == 0 if it worked
      mpu.CalibrateAccel(6);  // Calibration- generate offsets and calibrate MPU
      mpu.CalibrateGyro(6);
      mpu.PrintActiveOffsets();
      mpu.setDMPEnabled(true);  // turn on the DMP
      return 0;                 //It is all good, return 0
    } else return (devStatus);  //return an error number
  } else return (3);            //return error 3
  /* ERROR!
  1 = initial memory load failed
  2 = DMP configuration updates failed
  3 = MPU connection failed
  (if it's going to break, usually the code will be 1) */
}

// ================================================================
// ===         FUNCTIONS TO GET YAW, PITCH, AND ROLL            ===
// ================================================================
// displays Euler angles in degrees

float getYaw() {                                  //get the yaw of the drone
  MPU6050 mpu(MPU_ADDR);                          //Set address of MPU (library Dependancy)
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {  // Get the Latest packet
    mpu.dmpGetQuaternion(&q, fifoBuffer);         //get the Quaternions from the DMP
    mpu.dmpGetGravity(&gravity, &q);              //Get the gravity, to find the orientation of the Quaternions
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);    //Compute the Yaw, Pitch, and roll
    return (ypr[0] * 180 / M_PI);                 //return Yaw
  }
}

float getPitch() {                                //get the pitch of the drone
  MPU6050 mpu(MPU_ADDR);                          //Set address of MPU (library Dependancy)
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {  //get the latest data
    mpu.dmpGetQuaternion(&q, fifoBuffer);         //get the Quaternions from the DMP
    mpu.dmpGetGravity(&gravity, &q);              //Get the gravity, to find the orientation of the Quaternions
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);    //Compute the Yaw, Pitch, and roll
    return (ypr[1] * 180 / M_PI);                 //return Pitch
  }
}

float getRoll() {                                 //Get the roll of the drone
  MPU6050 mpu(MPU_ADDR);                          //Set address of MPU (library Dependancy)
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {  //get the latest data
    mpu.dmpGetQuaternion(&q, fifoBuffer);         //get the Quaternions from the DMP
    mpu.dmpGetGravity(&gravity, &q);              //Get the gravity, to find the orientation of the Quaternions
    mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);    //Compute the Yaw, Pitch, and roll
    return (ypr[2] * 180 / M_PI);                 //return Roll
  }
}



// ================================================================
// ===              FUNCTIONS TO GET ACCELERATION               ===
// ================================================================
// displays initial world-frame acceleration, adjusted to remove gravity
// and rotated based on known orientation from quaternion

float getXAccel() {                                       //Get the X-axis acceleration
  MPU6050 mpu(MPU_ADDR);                                  //Set address of MPU (library Dependancy)
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {          //Get the latest data
    mpu.dmpGetQuaternion(&q, fifoBuffer);                 //get the Quaternions from the DMP
    mpu.dmpGetAccel(&aa, fifoBuffer);                     //get the accelerations from the DMP
    mpu.dmpGetGravity(&gravity, &q);                      //get the Gravity consts from the DMP
    mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);        //Convert the acceleration into linear
    mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);  //Set the world reference frame
    return (aaWorld.x);                                   //return the accleration
  }
}

float getYAccel() {                                       //Get the Y-axis acceleration
  MPU6050 mpu(MPU_ADDR);                                  //Set address of MPU (library Dependancy)
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {          //Get the latest data
    mpu.dmpGetQuaternion(&q, fifoBuffer);                 //get the Quaternions from the DMP
    mpu.dmpGetAccel(&aa, fifoBuffer);                     //get the accelerations from the DMP
    mpu.dmpGetGravity(&gravity, &q);                      //get the Gravity consts from the DMP
    mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);        //Convert the acceleration into linear
    mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);  //Set the world reference frame
    return (aaWorld.y);                                   //return the accleration
  }
}

float getZAccel() {                                       //Get the Z-axis acceleration
  MPU6050 mpu(MPU_ADDR);                                  //Set address of MPU (library Dependancy)
  if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) {          //Get the latest data
    mpu.dmpGetQuaternion(&q, fifoBuffer);                 //get the Quaternions from the DMP
    mpu.dmpGetAccel(&aa, fifoBuffer);                     //get the accelerations from the DMP
    mpu.dmpGetGravity(&gravity, &q);                      //get the Gravity consts from the DMP
    mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);        //Convert the acceleration into linear
    mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);  //Set the world reference frame
    return (aaWorld.z);                                   //return the accleratoin
  }
}