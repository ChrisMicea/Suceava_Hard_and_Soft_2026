package org.example.backend.repository;

import org.example.backend.model.Motion;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface MotionRepository extends JpaRepository<Motion, Long> {
    Optional<Motion> findFirstByOrderByCreatedAtDesc();
}
