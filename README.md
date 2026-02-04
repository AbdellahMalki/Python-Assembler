# Projet informatique — Assembleur Python (.pys vers .pyc)

## Introduction

Ce dépôt contient le code nécessaire pour compiler un langage d’assemblage Python .pys, avec :
- un parseur de regex + matching,
- un lexer,
- un parser syntaxique produisant un objet pyobj,
- un assembleur + sérialisation pour générer un fichier .pyc (format Python 2.7).


## Structure du dépôt

Arborescence :

```text
.
├── Makefile
├── README.md
├── include/        # headers des modules
├── src/            # implémentations des modules
├── app/            # programmes principaux (livrables)
└── test/           # programmes de test + données + howto Unitest
```


Programmes principaux (répertoire `app/`) :
- `regexp-read` : lecture/parse d’une regexp en structure interne
- `regexp-match` : matching d’une regexp sur un texte
- `lex` : analyse lexicale d’un fichier source à partir de règles
- `parser` : analyse syntaxique d’un `.pys` (affiche la structure)
- `pyas` : assembleur `.pys` → `.pyc`


## Compiler

Depuis la racine du dépôt :

- Compiler et lancer la non-régression (tests) + compiler les programmes :
   ```bash
   make
   ```


## Exécuter

Les exécutables sont produits dans `app/`.


### Regex

- Parse une regexp, par exemple :
   ```bash
   ./app/regexp-read "[a-zA-Z_][a-zA-Z0-9_]*"
   ```

- Match une regexp sur un texte (match en préfixe, cf. message du programme), par exemple :
   ```bash
   ./app/regexp-match "ab*c" "abbbc"
   ```


### Lexer

`lex` attend : un fichier de règles (lexèmes) + un fichier source , par exemple :
```bash
./app/lex include/lexer/regexp_file.lex test/data/files-pys/4-simple.pys
```


### Parser

`parser` attend uniquement le fichier source `.pys` (il utilise les règles lexer par défaut : `include/lexer/regexp_file.lex`).

```bash
./app/parser test/data/files-pys/4-simple.pys
```


### Assembleur `.pys` vers `.pyc`

`pyas` attend : source `.pys` + chemin de sortie `.pyc` + fichier de règles lexer.

```bash
./app/pyas include/lexer/regexp_file.lex test/data/files-pys/4-simple.pys out.pyc 
```

Les dossiers `test/data/expected-pyc-output/` et `test/data/expected-pys/` contiennent des sorties attendues par les tests.


## Tester

Le dépôt utilise `Unitest`.

- Exécuter toute la suite de tests :
   ```bash
   make check
   ```
   Un rapport est généré dans `unitest.report`.

- Exécuter un test précis :
exemple : 
   ```bash
   make test/5-lexer@check
   ```


## Nettoyer

- Nettoyage :
   ```bash
   make clean
   ```


## Incréments du projet

- Incrément 0 : structures génériques (`list`, `queue`) + tests (`test/0-list`, `test/0b-queue`).
- Incrément 1 : regexp (parse + chargroup + matching) + tests (`test/1-regexp` à `test/4-regexp-match`) + programme `app/regexp-read` + programme `app/regexp-match`.
- Incrément 2 : lexer + test `test/5-lexer` + programme `app/lex`.
- Incrément 3 : parser + objets Python (`pyobj`) + test `test/6-pyobj`, `test/7-parser` + programme `app/parser`.
- Incrément 4 : génération `.pyc` (assembleur, sérialisation, lnotab) + tests `test/8-lnotab`, `test/9-pays` + programme `app/pyas`.


Merci à l'équipe enseignante pour ce projet !
