# Database Guidelines

> Database and persistent storage conventions for metaRTC.

---

## Overview

metaRTC is a real-time streaming SDK and does **not** use a relational database or ORM. Persistence is limited to:

- Configuration files (`yang_config.ini`)
- JSON-based signaling payloads
- Optional log files (`yang_log.log`)

If you are adding a feature that requires structured persistence, prefer simple file-based configuration or JSON rather than introducing a database dependency.

---

## Configuration Files

Configuration is typically loaded from `.ini` files via `YangIni` utilities. Example:

- Path: `demo/metapushstream7/yang_config.ini`
- Parser: `include/yangutil/sys/YangIni.h`

Keep configuration keys consistent with existing naming patterns and document new keys in the relevant demo README.

---

## JSON Payloads

Signaling and control messages use JSON. The project uses `jsoncpp` (headers under `thirdparty/include/json/`).

- Construct and parse JSON using `YangJson` abstraction where available.
- Keep payload shapes stable across versions because they are exchanged with external servers (SRS, ZLM, Janus, etc.).

---

## Log Files

Runtime logs may be written to `yang_log.log` in the working directory. See [Logging Guidelines](./logging-guidelines.md) for levels and formatting.

---

## When to Introduce a Database

This codebase is not designed for database-backed operations. If you think you need one, discuss it first — the intended pattern is stateless streaming with in-memory buffers and optional INI/JSON configuration.

---

## Forbidden Patterns

- Do not add SQLite, LevelDB, or other embedded database dependencies without explicit approval.
- Do not store user credentials or stream keys in plain-text configuration files.
