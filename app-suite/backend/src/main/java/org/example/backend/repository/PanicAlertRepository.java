package org.example.backend.repository;

import org.example.backend.model.PanicAlert;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;
import java.util.Optional;

@Repository
public interface PanicAlertRepository extends JpaRepository<PanicAlert, Long> {
    Optional<PanicAlert> findFirstByStatusOrderByCreatedAtDesc(String status);
    List<PanicAlert> findByStatusOrderByCreatedAtDesc(String status);
}
