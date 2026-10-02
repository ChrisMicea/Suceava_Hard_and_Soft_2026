package org.example.backend.dto;

public class PanicAlertRequest {
    private String eventType;

    public PanicAlertRequest() {}

    public PanicAlertRequest(String eventType) {
        this.eventType = eventType;
    }

    public String getEventType() {
        return eventType;
    }

    public void setEventType(String eventType) {
        this.eventType = eventType;
    }
}
