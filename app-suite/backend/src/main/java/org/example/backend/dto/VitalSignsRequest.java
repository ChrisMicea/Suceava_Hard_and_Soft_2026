package org.example.backend.dto;

public class VitalSignsRequest {
    private Integer heartrate;
    private Float oxygen;
    private Float confidence;

    public VitalSignsRequest() {}

    public VitalSignsRequest(Integer heartrate, Float oxygen, Float confidence) {
        this.heartrate = heartrate;
        this.oxygen = oxygen;
        this.confidence = confidence;
    }

    public Integer getHeartrate() {
        return heartrate;
    }

    public void setHeartrate(Integer heartrate) {
        this.heartrate = heartrate;
    }

    public Float getOxygen() {
        return oxygen;
    }

    public void setOxygen(Float oxygen) {
        this.oxygen = oxygen;
    }

    public Float getConfidence() {
        return confidence;
    }

    public void setConfidence(Float confidence) {
        this.confidence = confidence;
    }
}
