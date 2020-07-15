BEGIN TRANSACTION;

CREATE TABLE zs_function_parameter (
    identifier_id INTEGER REFERENCES zs_identifier (id) ON DELETE CASCADE,
    [index]       INTEGER,
    name          TEXT,
    type          TEXT,
    default_value TEXT,
    UNIQUE (
        identifier_id,
        [index]
    )
);

CREATE INDEX zs_function_parameter_identifier_id ON zs_function_parameter (identifier_id);

COMMIT TRANSACTION;
