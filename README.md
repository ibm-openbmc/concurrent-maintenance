# Concurrent Maintenance

## Overview

Concurrent Maintenance (CM) is the capability to perform hardware maintenance or
replacement activities on supported platform components while the system remains
operational. This is a guided flow.

The Concurrent Maintenance daemon provides a centralized framework within
OpenBMC for managing Concurrent Maintenance operations and tracking their
progress through D-Bus interfaces. The daemon coordinates requests originating
from external management applications and orchestrates interactions with
platform-specific services responsible for executing maintenance procedures.

---

## Motivation

Modern enterprise systems support maintenance of selected hardware components
without requiring a full system shutdown. These maintenance operations often
involve coordination across multiple software and hardware components,
including:

- Platform inventory services
- Hardware management applications
- VPD management services
- PLDM-based communication services
- External management applications

The Concurrent Maintenance daemon serves as a central orchestration point for
these workflows and provides a consistent interface for tracking maintenance
operations and their lifecycle.

---

## Architecture

```text
External Management Application
            |
            v
      Redfish / APIs
            |
            v
+---------------------------------------------+
|           BMC                               |
|   +--------------------------------+        |
|   | Concurrent Maintenance Daemon  |        |
|   +--------------------------------+        |
|                       |                     |
|           +--------------------+            |
|           |                    |            |
|           v                    v            |
|    Inventory Services    Platform Services  |
|           |                    |            |
|           +--------------------+            |
+---------------------------------------------+
                        |
                        v
                  Hardware Actions

```

The daemon is responsible for:

1. Receiving Concurrent Maintenance requests.
2. Target FRU Lookup (or identification) from Inventory Manager.
3. Concurrent maintenance object lifecycle.
4. Coordinating with platform-specific services.
5. Managing and exposing maintenance state through D-Bus interfaces.

---

## Supported Maintenance Types

The framework is designed to support different types of Concurrent Maintenance
workflows, including:

- FSI Concurrent Maintenance
- BMC Concurrent Maintenance
- Switchboard Concurrent Maintenance

Additional maintenance types may be added in future enhancements.

---

## D-Bus Service

The daemon owns the following D-Bus service name:

```text
com.ibm.ConcurrentMaintenance
```

---

## Build

```bash
meson setup build
meson compile -C build
```

---

## Service

The daemon is installed as:

```text
com.ibm.ConcurrentMaintenance.service
```

and launches:

```text
/usr/bin/ibm-concurrent-maintenance
```
