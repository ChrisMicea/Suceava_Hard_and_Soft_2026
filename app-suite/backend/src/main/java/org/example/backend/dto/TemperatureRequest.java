package org.example.backend.dto;

public class TemperatureRequest {
    private Float temperature;

    public TemperatureRequest() {}

    public TemperatureRequest(Float temperature) {
        this.temperature = temperature;
    }

    public Float getTemperature() {
        return temperature;
    }

    public void setTemperature(Float temperature) {
        this.temperature = temperature;
    }
}
