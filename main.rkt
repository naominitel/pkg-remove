#lang racket

(require ffi/unsafe)
(require ffi/unsafe/define)
(require racket/gui/base)

(define PKGDIR "/var/db/receipts")

;; import pkg_remove from C code

(define-ffi-definer define-pkg (ffi-lib "bom-remove/pkg-remove"))
(define-pkg pkg_remove (_fun _string -> _int))

;; list-pkgs :: (listof path?)
;; get installed packages list

(define (list-pkgs)
    (filter 
        (lambda (p) (regexp-match ".bom$" p))
        (directory-list (string->path PKGDIR))))

;; pkg-name :: path? -> string?
;; return the name of a package : /var/db/receipts/package.bom -> package

(define (pkg-name p)
    (let-values 
        (((base name dir) (split-path (path-replace-suffix p ""))))
        (path->string name)))

;; create a Window and the package list

(define pkgs (list-pkgs))
(define window (new frame% [label "Packages"]))

;; create a list box for storing the package list

(define list (new list-box% 
    [choices (map pkg-name (list-pkgs))]
    [parent window]
    [label ""]))

;; insert data in the list box (package name)

(let ((i 0))
    (map (lambda (p) 
            (send list set-data i (pkg-name p))
            (set! i (+ i 1))) 
          pkgs))

;; create a button

(define btn-ok (new button%
    [callback on-click]
    [parent window]
    [label "Uninstall"]))

;; on click: get the selected package and call C code

(define (on-click button ev)
    (let ((sel (send list get-selections)))
        (if (null? sel)
            (void)
            (pkg_remove (send list get-data (car sel))))))

;; run the main event loop

(send window show #t)
