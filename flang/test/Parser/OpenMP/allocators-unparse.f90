! RUN: %flang_fc1 -fdebug-unparse-no-sema -fopenmp %s | FileCheck --ignore-case %s
! RUN: %flang_fc1 -fdebug-dump-parse-tree-no-sema -fopenmp %s | FileCheck --check-prefix="PARSE-TREE" %s
! Check unparsing of OpenMP 5.2 Allocators construct

subroutine allocate()
  use omp_lib
  integer, allocatable :: arr1(:), arr2(:, :)

  !$omp allocators allocate(omp_default_mem_alloc: arr1)
    allocate(arr1(5))

  !$omp allocators allocate(allocator(omp_default_mem_alloc), align(32): arr1) &
  !$omp allocate(omp_default_mem_alloc: arr2)
    allocate(arr1(10), arr2(3, 2))

  !$omp allocators allocate(align(32): arr2)
    allocate(arr2(5, 3))
  !$omp end allocators
end subroutine allocate

!CHECK: INTEGER, ALLOCATABLE :: arr1(:), arr2(:,:)
!CHECK-NEXT:!$OMP ALLOCATORS ALLOCATE(omp_default_mem_alloc: arr1)
!CHECK-NEXT: ALLOCATE(arr1(5))
!CHECK-NEXT:!$OMP ALLOCATORS ALLOCATE(ALLOCATOR(omp_default_mem_alloc), ALIGN(32): arr1) A&
!CHECK-NEXT:!$OMP&LLOCATE(omp_default_mem_alloc: arr2)
!CHECK-NEXT: ALLOCATE(arr1(10), arr2(3,2))
!CHECK-NEXT:!$OMP ALLOCATORS ALLOCATE(ALIGN(32): arr2)
!CHECK-NEXT: ALLOCATE(arr2(5,3))

!PARSE-TREE: ExecutionPartConstruct -> ExecutableConstruct -> OpenMPConstruct -> OpenMPAllocatorsConstruct
!PARSE-TREE-NEXT: | OmpBeginDirective
!PARSE-TREE-NEXT: | | OmpDirectiveName -> llvm::omp::Directive = allocators
!PARSE-TREE-NEXT: | | OmpClauseList -> OmpClause -> Allocate -> OmpAllocateClause
!PARSE-TREE-NEXT: | | | Modifier -> OmpAllocatorSimpleModifier -> Scalar -> Integer -> Expr -> Designator -> DataRef -> Name = 'omp_default_mem_alloc'
!PARSE-TREE-NEXT: | | | OmpObjectList -> OmpObject -> Designator -> DataRef -> Name = 'arr1'
!PARSE-TREE-NEXT: | | Flags = None
!PARSE-TREE-NEXT: | Block
!PARSE-TREE-NEXT: | | ExecutionPartConstruct -> ExecutableConstruct -> ActionStmt -> AllocateStmt
!PARSE-TREE-NEXT: | | | Allocation
!PARSE-TREE-NEXT: | | | | AllocateObject -> Name = 'arr1'

!PARSE-TREE: ExecutionPartConstruct -> ExecutableConstruct -> OpenMPConstruct -> OpenMPAllocatorsConstruct
!PARSE-TREE-NEXT: | OmpBeginDirective
!PARSE-TREE-NEXT: | | OmpDirectiveName -> llvm::omp::Directive = allocators
!PARSE-TREE-NEXT: | | OmpClauseList -> OmpClause -> Allocate -> OmpAllocateClause
!PARSE-TREE-NEXT: | | | Modifier -> OmpAllocatorComplexModifier -> Scalar -> Integer -> Expr -> Designator -> DataRef -> Name = 'omp_default_mem_alloc'
!PARSE-TREE-NEXT: | | | Modifier -> OmpAlignModifier -> Scalar -> Integer -> Expr -> LiteralConstant -> IntLiteralConstant = '32'
!PARSE-TREE-NEXT: | | | OmpObjectList -> OmpObject -> Designator -> DataRef -> Name = 'arr1'
!PARSE-TREE-NEXT: | | OmpClause -> Allocate -> OmpAllocateClause
!PARSE-TREE-NEXT: | | | Modifier -> OmpAllocatorSimpleModifier -> Scalar -> Integer -> Expr -> Designator -> DataRef -> Name = 'omp_default_mem_alloc'
!PARSE-TREE-NEXT: | | | OmpObjectList -> OmpObject -> Designator -> DataRef -> Name = 'arr2'
!PARSE-TREE-NEXT: | | Flags = None
!PARSE-TREE-NEXT: | Block
!PARSE-TREE-NEXT: | | ExecutionPartConstruct -> ExecutableConstruct -> ActionStmt -> AllocateStmt
!PARSE-TREE-NEXT: | | | Allocation
!PARSE-TREE-NEXT: | | | | AllocateObject -> Name = 'arr1'

!PARSE-TREE: ExecutionPartConstruct -> ExecutableConstruct -> OpenMPConstruct -> OpenMPAllocatorsConstruct
!PARSE-TREE-NEXT: | OmpBeginDirective
!PARSE-TREE-NEXT: | | OmpDirectiveName -> llvm::omp::Directive = allocators
!PARSE-TREE-NEXT: | | OmpClauseList -> OmpClause -> Allocate -> OmpAllocateClause
!PARSE-TREE-NEXT: | | | Modifier -> OmpAlignModifier -> Scalar -> Integer -> Expr -> LiteralConstant -> IntLiteralConstant = '32'
!PARSE-TREE-NEXT: | | | OmpObjectList -> OmpObject -> Designator -> DataRef -> Name = 'arr2'
!PARSE-TREE-NEXT: | | Flags = None
!PARSE-TREE-NEXT: | Block
!PARSE-TREE-NEXT: | | ExecutionPartConstruct -> ExecutableConstruct -> ActionStmt -> AllocateStmt
!PARSE-TREE-NEXT: | | | Allocation
!PARSE-TREE-NEXT: | | | | AllocateObject -> Name = 'arr2'
!PARSE-TREE-NEXT: | | | | AllocateShapeSpec
!PARSE-TREE-NEXT: | | | | | Scalar -> Integer -> Expr -> LiteralConstant -> IntLiteralConstant = '5'
!PARSE-TREE-NEXT: | | | | AllocateShapeSpec
!PARSE-TREE-NEXT: | | | | | Scalar -> Integer -> Expr -> LiteralConstant -> IntLiteralConstant = '3'
!PARSE-TREE-NEXT: | OmpEndDirective
!PARSE-TREE-NEXT: | | OmpDirectiveName -> llvm::omp::Directive = allocators
!PARSE-TREE-NEXT: | | OmpClauseList ->
!PARSE-TREE-NEXT: | | Flags = None
