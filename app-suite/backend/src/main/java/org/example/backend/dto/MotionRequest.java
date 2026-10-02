package org.example.backend.dto;

public class MotionRequest {
    private Float rotX;
    private Float rotY;
    private Float rotZ;
    private Float accX;
    private Float accY;
    private Float accZ;

    public MotionRequest() {}

    public MotionRequest(Float rotX, Float rotY, Float rotZ, Float accX, Float accY, Float accZ) {
        this.rotX = rotX;
        this.rotY = rotY;
        this.rotZ = rotZ;
        this.accX = accX;
        this.accY = accY;
        this.accZ = accZ;
    }

    public Float getRotX() {
        return rotX;
    }

    public void setRotX(Float rotX) {
        this.rotX = rotX;
    }

    public Float getRotY() {
        return rotY;
    }

    public void setRotY(Float rotY) {
        this.rotY = rotY;
    }

    public Float getRotZ() {
        return rotZ;
    }

    public void setRotZ(Float rotZ) {
        this.rotZ = rotZ;
    }

    public Float getAccX() {
        return accX;
    }

    public void setAccX(Float accX) {
        this.accX = accX;
    }

    public Float getAccY() {
        return accY;
    }

    public void setAccY(Float accY) {
        this.accY = accY;
    }

    public Float getAccZ() {
        return accZ;
    }

    public void setAccZ(Float accZ) {
        this.accZ = accZ;
    }
}
