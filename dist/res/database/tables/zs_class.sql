BEGIN TRANSACTION;

CREATE TABLE zs_class (
    identifier_id INTEGER REFERENCES zs_identifier (id) ON DELETE CASCADE
                          PRIMARY KEY,
    scope_id      INTEGER REFERENCES zs_object_scope (id),
    base_class    TEXT,
    abstract      BOOLEAN,
    native        BOOLEAN,
    replaces      TEXT,
    version       TEXT
);

COMMIT TRANSACTION;
