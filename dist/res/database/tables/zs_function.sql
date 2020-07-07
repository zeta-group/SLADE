BEGIN TRANSACTION;

CREATE TABLE zs_function (
    identifier_id INTEGER REFERENCES zs_identifier (id) ON DELETE CASCADE,
    scope_id      INTEGER REFERENCES zs_object_scope (id),
    return_type   TEXT,
    visibility    INTEGER,
    [action]      BOOLEAN,
    action_scope  TEXT,
    const         BOOLEAN,
    final         BOOLEAN,
    native        BOOLEAN,
    override      BOOLEAN,
    static        BOOLEAN,
    vararg        BOOLEAN,
    [virtual]     BOOLEAN,
    virtualscope  BOOLEAN,
    deprecated    TEXT,
    version       TEXT
);

COMMIT TRANSACTION;
