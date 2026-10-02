package org.example.backend.model;

import jakarta.persistence.*;
import java.time.LocalDateTime;

@Entity
@Table(name = "motion")
public class Motion {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "rot_x", nullable = false)
    private Float rotX;

    @Column(name = "rot_y", nullable = false)
    private Float rotY;

    @Column(name = "rot_z", nullable = false)
    private Float rotZ;

    @Column(name = "acc_x", nullable = false)
    private Float accX;

    @Column(name = "acc_y", nullable = false)
    private Float accY;

    @Column(name = "acc_z", nullable = false)
    private Float accZ;

    @Column(nullable = false, updatable = false)
    private LocalDateTime createdAt;

    @PrePersist
    protected void onCreate() {
        createdAt = LocalDateTime.now();
    }

    public Motion() {}

    public Motion(Float rotX, Float rotY, Float rotZ, Float accX, Float accY, Float accZ) {
        this.rotX = rotX;
        this.rotY = rotY;
        this.rotZ = rotZ;
        this.accX = accX;
        this.accY = accY;
        this.accZ = accZ;
    }

    public Long getId() {
        return id;
    }

    public void setId(Long id) {
        this.id = id;
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

    public LocalDateTime getCreatedAt() {
        return createdAt;
    }

    public void setCreatedAt(LocalDateTime createdAt) {
        this.createdAt = createdAt;
    }
}
