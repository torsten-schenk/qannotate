# QAnnotate

This software has been created within the project [Humanist Computer Interaction auf dem Prüfstand](https://humanist.hs-mainz.de).
Its purpose is to enable historians to create and edit a database containing a variety of entities and to link text passages to the entities.
The file system is used as database backend to enable database management using Git. The file format has been crafted
to be well integrateable into a Git repository.

## Disclaimer

The current version is highly specialized to the project's requirements,
namely commenting letters and automatically generating the book [TODO add book title](TODO add link).

Within the course of the project requirements towards this software changed severely, which lead to an
incoherent design making some kinds of changes easily integrateable and others almost impossible.
We nevertheless believe that the core idea of the project -- creating a configurable GUI with a Git database backend --
is a goal worth pursuing, so there likely will be a follow-up project.
Despite these inconveniences, feel free to experiment with the current version, the binary as well as the codebase.

## Getting Started

We currently support steps to get started using a precompiled windows binary. If you want to build the software yourself,
[build.sh](build.sh) contains the instructions in a very basic way.

### Creating Database Folder

The database resides within a folder on your hard drive. The file [qannotate.ini](qannotate.ini) within the folder
identifies the database, so you don't accidentally open an incompatible/a non-database folder. The following steps will
setup a new database:

* Create an empty folder at a convenient location.
* Put the file [qannotate.ini](qannotate.ini) into that folder.

### Launching QAnnotate

* You can download the current precompiled windows version [here](TODO add link). No installation is required,
just launch the file.

* In the menu ``File`` &larr; ``Open``, select the database folder you created earlier.

### Adding/Editing Objects

If the previous steps completed successfully, you should see an object tree on the left side.

* To create a new object respectively a new subcategory, right-click on an item in the object tree.
* To edit an existing object, double-click the object in the object tree.

### Annotating Text

The annotatable original texts can be found within the category ``Texts`` &larr; ``Liber \<number\>`` in the object tree.
Whenever you create a letter within that category, you will be asked to enter the content text alongside the letter number.

The following steps will create a new annotatable text:

* Create a new book within ``Texts`` in the object tree. Assign a number to the book.
* Create a new letter within the subcategory ``Liber \<number\>``.
* Assign a number to the letter and paste the text to be annotated into the ``Content`` field.
* Double-clicking the resulting subobject ``Epistula \<number\>`` will now open the GUI with the annotatable text.

### Referencing Objects

QAnnotate itself does not incorporate the concept of cross-reference links. It is merely designed
to act as a window into the files within the database folder. Within the Humanist Computer Interaction project
we use a modified form of the [Wikipedia-Syntax](https://meta.wikimedia.org/wiki/Wiki_syntax) which is processed
by a subsequent script.

## License
This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments
* Most of the GUI work within the project is done by Christoph Kalchreuter.
