package org.example.backend.repository;

import org.example.backend.model.VitalSigns;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface VitalSignsRepository extends JpaRepository<VitalSigns, Long> {
    Optional<VitalSigns> findFirstByOrderByCreatedAtDesc();
}
