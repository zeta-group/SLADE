BEGIN TRANSACTION;

CREATE TABLE zs_identifier_type (
    id   INTEGER PRIMARY KEY,
    name TEXT
);

INSERT INTO zs_identifier_type (id, name) VALUES (1, 'variable');
INSERT INTO zs_identifier_type (id, name) VALUES (2, 'class');
INSERT INTO zs_identifier_type (id, name) VALUES (3, 'struct');
INSERT INTO zs_identifier_type (id, name) VALUES (4, 'enum');
INSERT INTO zs_identifier_type (id, name) VALUES (5, 'const');
INSERT INTO zs_identifier_type (id, name) VALUES (6, 'function');
INSERT INTO zs_identifier_type (id, name) VALUES (7, 'state');

COMMIT TRANSACTION;
