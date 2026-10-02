package org.example.backend.repository;

import org.example.backend.model.Temperature;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface TemperatureRepository extends JpaRepository<Temperature, Long> {
    Optional<Temperature> findFirstByOrderByCreatedAtDesc();
}
