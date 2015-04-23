(in-package :robray)

(defparameter *pov-output* *standard-output*)
(defparameter *pov-handedness* :left)

(defparameter *pov-indent* "")
(defparameter *pov-indent-width* 3) ;; as used in povray.org docs

;; Convert something to a POV-ray object

(defmacro with-pov-indent (old-indent &body body)
  `(let* ((,old-indent *pov-indent*)
          (*pov-indent* (make-string (+ *pov-indent-width* (length *pov-indent*))
                                     :initial-element #\Space)))
     ,@body))

;; (defmacro with-pov-block (output &body body)
;;   `(progn
;;      (write-char #\{ ,output)
;;      (let ((*pov-indent* (make-string (+ *pov-indent-width* (length *pov-indent*))
;;                                       :initial-element #\Space)))
;;        ,@body)
;;      (format ,output "~&~A}" *pov-indent*)))

(defstruct (pov-float (:constructor pov-float (value)))
  (value 0 :type double-float))

(defmethod print-object ((object pov-float) stream)
  (format stream "~F"
          (pov-float-value object)))

(defstruct (pov-float-vector (:constructor %pov-float-vector (x y z)))
  "Type for a povray vector."
  (x 0d0 :type double-float)
  (y 0d0 :type double-float)
  (z 0d0 :type double-float))

(defstruct pov-matrix
  elements)

(defun pov-matrix (tf)
  (let* ((matrix (amino::matrix-data (rotation-matrix (rotation tf))))
         (translation (translation tf)))
    ;; Swap the Y and Z axes because povray is left-handed
    (with-vec3 (x y z) translation
      (make-pov-matrix :elements
                       (list (aref matrix 0) (aref matrix 2) (aref matrix 1)
                             (aref matrix 6) (aref matrix 8) (aref matrix 7)
                             (aref matrix 3) (aref matrix 5) (aref matrix 4)
                             x z y)))))

(defmethod print-object ((object pov-matrix) stream)
  (let ((elements (pov-matrix-elements object)))
    (with-pov-indent old-indent
      (format stream "~&~Amatrix <~&~A~F, ~F, ~F,~&~A~F, ~F, ~F,~&~A~F, ~F, ~F,~&~A~F, ~F, ~F~&~A>"
              old-indent
              *pov-indent*
              (elt elements 0)
              (elt elements 1)
              (elt elements 2)
              *pov-indent*
              (elt elements 3)
              (elt elements 4)
              (elt elements 5)
              *pov-indent*
              (elt elements 6)
              (elt elements 7)
              (elt elements 8)
              *pov-indent*
              (elt elements 9)
              (elt elements 10)
              (elt elements 11)
              old-indent))))

(defstruct (pov-integer-vector (:constructor %pov-integer-vector (x y z)))
  "Type for a povray vector."
  (x 0 :type fixnum)
  (y 0 :type fixnum)
  (z 0 :type fixnum))

(defun pov-float-vector (elements)
  (%pov-float-vector (vec-x elements)
                     (vec-y elements)
                     (vec-z elements)))

(defun pov-float-vector-right (elements)
  (%pov-float-vector (vec-x elements)
                     (vec-z elements)
                     (vec-y elements)))

(defun pov-integer-vector (elements)
  (%pov-integer-vector (vec-x elements)
                       (vec-y elements)
                       (vec-z elements)))

(defmethod print-object ((object pov-float-vector) stream)
  (format stream "~&~A<~F, ~F, ~F>"
          *pov-indent*
          (pov-float-vector-x object)
          (pov-float-vector-y object)
          (pov-float-vector-z object)))

(defmethod print-object ((object pov-integer-vector) stream)
  (format stream "~&~A<~D, ~D, ~D>"
          *pov-indent*
          (pov-integer-vector-x object)
          (pov-integer-vector-y object)
          (pov-integer-vector-z object)))

(defstruct (pov-value (:constructor pov-value (value)))
  value)

(defmethod print-object ((object pov-value) stream)
  (format stream "~&~A~A"
          *pov-indent*
          (pov-value-value object)))


(defstruct (pov-block (:constructor pov-block (name list)))
  name
  list)

(defmethod print-object ((object pov-block) stream)
  (with-pov-indent old-indent
    (format stream
            "~&~A~A {~&~{~&~A~}~&~A}"
            old-indent
            (pov-block-name object)
            (pov-block-list object)
            old-indent)))

(defstruct (pov-list (:constructor %pov-list (name list length)))
  name
  length
  list)

(defun pov-list (name list &optional (length (length list)))
  (%pov-list name list length))

(defmethod print-object ((object pov-list) stream)
  (with-pov-indent old-indent
    (format stream
            "~&~A~A {~D,~{~A~^,~}~&~A}"
            old-indent
            (pov-list-name object)
            (pov-value (pov-list-length object))
            (pov-list-list object)
            old-indent)))

(defstruct (pov-item (:constructor pov-item (name value)))
  name
  value)

(defstruct (pov-rgb (:constructor %pov-rgb (r g b)))
  "Type for a povray RGB color."
  (r 0d0 :type double-float)
  (g 0d0 :type double-float)
  (b 0d0 :type double-float))

(defun pov-rgb* (r g b)
  (%pov-rgb (coerce r 'double-float)
            (coerce g 'double-float)
            (coerce b 'double-float)))

(defun pov-rgb (elements)
  (apply #'pov-rgb* (subseq elements 0 3)))

(defmethod print-object ((object pov-rgb) stream)
  (format stream "rgb<~F, ~F, ~F>"
          (pov-rgb-r object)
          (pov-rgb-g object)
          (pov-rgb-b object)))

(defmethod print-object ((object pov-item) stream)
  (format stream "~&~A~A ~A"
          *pov-indent* (pov-item-name object) (pov-item-value object)))

(defmacro def-pov-block (name)
  (let ((name (string-downcase (string name))))
    (let ((fun (intern (string-upcase (concatenate 'string "pov-" name))))
          (fun* (intern (string-upcase (concatenate 'string "pov-" name "*")))))
      `(progn
         (defun ,fun (things)
           (pov-block ,name (ensure-list things)))
         (defun ,fun* (&rest things)
           (,fun things))))))

(def-pov-block texture)
(def-pov-block finish)
(def-pov-block pigment)
(def-pov-block transform)


(defun pov-box (first-corner second-corner &optional modifiers)
  (pov-block "box" (list* first-corner second-corner modifiers)))

(defun pov-sphere (center radius &optional modifiers)
  (pov-block "sphere"
             (list* (pov-float-vector-right center)
                    (pov-value (pov-float radius))
                    modifiers)))


(defun pov-box-center (dimensions
                       &key modifiers)
  (let* ((first-corner-vec (g* 0.5 dimensions))
         (second-corner-vec (g* -.05 dimensions)))
    (pov-box (pov-float-vector-right first-corner-vec)
             (pov-float-vector-right second-corner-vec)
             modifiers)))


(defun pov-mesh2 (&key
                    vertex-vectors
                    face-indices
                    texture-list
                    normal-vectors
                    normal-indices
                    modifiers
                    mesh
                    matrix
                    )
  "Create a povray mesh2 object.

VERTEX-VECTORS: List of vertices in the mesh as pov-vertex
FACE-INDICES: List of vertex indices for each triangle, as pov-vertex
"
  (declare (ignore normal-vectors normal-indices))
  (let ((args modifiers))
    ;; TODO: figure this out
    ;; (when normal-indices
    ;;   (push (pov-list "normal_indices" normal-indices) args))
    ;; (when normal-vectors
    ;;   (push (pov-list "normal_vectors" normal-vectors) args))
    (when matrix
      (push (pov-matrix matrix) args))
    (when mesh
      (push (pov-value mesh) args))

    (when face-indices
      (push (pov-list "face_indices"
                      face-indices
                      (if texture-list
                          (/ (length face-indices) 2)
                          (length face-indices)))
            args))
    (when texture-list
      (push (pov-texture-list texture-list) args))
    (when vertex-vectors
      (push (pov-list "vertex_vectors" vertex-vectors) args))

    (pov-block "mesh2" args)))

(defun pov-texture-list (textures)
  (pov-list "texture_list" textures))


(defstruct (pov-version (:constructor pov-version (value)))
  value)

(defmethod print-object ((object pov-version) stream)
  (with-pov-indent old-indent
    (declare (ignore old-indent))
    (format stream "~&#version ~A;"
            (pov-version-value object))))

(defstruct (pov-directive (:constructor pov-directive (type name value)))
  type
  name
  value)

(defun pov-declare (name value)
  (pov-directive "declare" name value))

(defmethod print-object ((object pov-directive) stream)
  (with-pov-indent old-indent
    (declare (ignore old-indent))
    (format stream "~&#~A ~A = ~A"
            (pov-directive-type object)
            (pov-directive-name object)
            (pov-directive-value object))))


(defstruct (pov-include (:constructor pov-include (file)))
  file)

(defmethod print-object ((object pov-include) stream)
  (format stream "~&#include ~S"
          (pov-include-file object)))

(defstruct (pov-sequence (:constructor pov-sequence (statements)))
  statements)

(defmethod print-object ((object pov-sequence) stream)
  (loop for x in (pov-sequence-statements object)
     do (print-object x stream)))

(defun pov-quality (float-quality)
  "Return numeric povray quality for proportional to RATIO.
RATIO: Floating point quality value in the range [0,1]"
  (clamp (round (* float-quality 11))
         0 11))


(defstruct (pov-if= (:constructor pov-if= (variable value statements)))
  variable
  value
  statements)

(defstruct (pov-switch (:constructor pov-switch (value clauses &optional else)))
  value
  clauses
  else)

(defmethod print-object ((object pov-switch) stream)
  (with-pov-indent old-indent
    (declare (ignore old-indent))
    (format stream "~&#switch (~A) ~{~A~}~@[~A~]~&#end"
            (pov-switch-value object)
            (pov-switch-clauses object)
            (pov-switch-else object))))

(defstruct (pov-case (:constructor pov-case (value statements)))
  value
  statements)

(defmethod print-object ((object pov-case) stream)
  (with-pov-indent old-indent
    (format stream "~&~A#case (~A) ~{~A~}~&~A#break"
            old-indent
            (pov-case-value object) (pov-case-statements object)
            old-indent)))

;(defmethod print-object

(defun pov-args (file
                 &key
                   output
                   (antialias t)
                   (width *width*)
                   (height *height*)
                   (quality *quality*)
                   verbose
                   threads
                   other)
  `(,(namestring file)
     ,@(when output (list (format nil "+O~A" output)))
     "-D" ; don't invoke display
     "-GS"
     ,@(when antialias (list "+A")) ; anti-alias
     ,@(when threads (list (format nil "+WT~D" threads)))
     ,(if verbose "+V" "-V")
     ,(format nil "+Q~D" (pov-quality quality))
     ,(format nil "+W~D" width)
     ,(format nil "+H~D" height)
     ,@other))

(defun pov-output-file (pov-file &optional (suffix ".png"))
  (ppcre:regex-replace "\.pov$" (namestring pov-file) suffix))

(defun pov-render (things
                   &key
                     (file "/tmp/robray.pov")
                     (output "/tmp/robray.png")
                     (width *width*)
                     (height *height*)
                     (quality *quality*))
  (let ((things (if (listp things)
                    (pov-sequence things)
                    things))
        (quality (pov-quality quality)))
    ;; write output
    (output things file)
    ;; run povray
    ;povray frame.pov -D +O/tmp/pov.png +A +H1080 +W1920
    (let ((args (list file
                      (format nil "+O~A" output)
                      "-D" ; don't invoke display
                      "+A" ; anti-alias
                      (format nil "+Q~D" quality)
                      (format nil "+W~D" width)
                      (format nil "+H~D" height))))
      (format t "~&Running: povray ~{~A~^ ~}" args)
      (sb-ext:run-program "povray" args :search t)
      (format t "~&done"))))

(defun pov-make-tarball (directory file)
  (sb-ext:run-program (find-script "make-tarball")
                      (list file)
                      :search nil :wait t
                      :directory directory
                      :error *error-output*))
