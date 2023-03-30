; ModuleID = 'person.bc'
source_filename = "person.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.clothes = type { [10 x i8], i32 }
%struct.person_t = type { i32, [10 x i8], %struct.clothes* }

@clo = dso_local global %struct.clothes { [10 x i8] c"yellow\00\00\00\00", i32 2 }, align 4, !dbg !0
@p = dso_local global %struct.person_t { i32 10, [10 x i8] c"bob\00\00\00\00\00\00\00", %struct.clothes* @clo }, align 8, !dbg !6
@__const.main.c = private unnamed_addr constant %struct.clothes { [10 x i8] c"red\00\00\00\00\00\00\00", i32 5 }, align 4
@.str = private unnamed_addr constant [10 x i8] c"Jack\00\00\00\00\00\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local %struct.person_t* @f(%struct.person_t* %0) #0 !dbg !29 {
  %2 = alloca %struct.person_t*, align 8
  store %struct.person_t* %0, %struct.person_t** %2, align 8
  call void @llvm.dbg.declare(metadata %struct.person_t** %2, metadata !33, metadata !DIExpression()), !dbg !34
  %3 = load %struct.person_t*, %struct.person_t** %2, align 8, !dbg !35
  %4 = getelementptr inbounds %struct.person_t, %struct.person_t* %3, i32 0, i32 0, !dbg !36
  %5 = load i32, i32* %4, align 8, !dbg !37
  %6 = add nsw i32 %5, 1, !dbg !37
  store i32 %6, i32* %4, align 8, !dbg !37
  %7 = load i32, i32* getelementptr inbounds (%struct.person_t, %struct.person_t* @p, i32 0, i32 0), align 8, !dbg !38
  %8 = add nsw i32 %7, 1, !dbg !38
  store i32 %8, i32* getelementptr inbounds (%struct.person_t, %struct.person_t* @p, i32 0, i32 0), align 8, !dbg !38
  %9 = load %struct.person_t*, %struct.person_t** %2, align 8, !dbg !39
  ret %struct.person_t* %9, !dbg !40
}

; Function Attrs: nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 !dbg !41 {
  %1 = alloca i32, align 4
  %2 = alloca %struct.clothes, align 4
  %3 = alloca %struct.person_t, align 8
  %4 = alloca %struct.person_t*, align 8
  %5 = alloca %struct.person_t*, align 8
  %6 = alloca %struct.person_t*, align 8
  store i32 0, i32* %1, align 4
  call void @llvm.dbg.declare(metadata %struct.clothes* %2, metadata !44, metadata !DIExpression()), !dbg !45
  %7 = bitcast %struct.clothes* %2 to i8*, !dbg !45
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %7, i8* align 4 getelementptr inbounds (%struct.clothes, %struct.clothes* @__const.main.c, i32 0, i32 0, i32 0), i64 16, i1 false), !dbg !45
  call void @llvm.dbg.declare(metadata %struct.person_t* %3, metadata !46, metadata !DIExpression()), !dbg !47
  %8 = getelementptr inbounds %struct.person_t, %struct.person_t* %3, i32 0, i32 0, !dbg !48
  store i32 10, i32* %8, align 8, !dbg !48
  %9 = getelementptr inbounds %struct.person_t, %struct.person_t* %3, i32 0, i32 1, !dbg !48
  %10 = bitcast [10 x i8]* %9 to i8*, !dbg !49
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %10, i8* align 1 getelementptr inbounds ([10 x i8], [10 x i8]* @.str, i32 0, i32 0), i64 10, i1 false), !dbg !49
  %11 = getelementptr inbounds %struct.person_t, %struct.person_t* %3, i32 0, i32 2, !dbg !48
  store %struct.clothes* %2, %struct.clothes** %11, align 8, !dbg !48
  call void @llvm.dbg.declare(metadata %struct.person_t** %4, metadata !50, metadata !DIExpression()), !dbg !51
  store %struct.person_t* %3, %struct.person_t** %4, align 8, !dbg !51
  call void @llvm.dbg.declare(metadata %struct.person_t** %5, metadata !52, metadata !DIExpression()), !dbg !53
  %12 = load %struct.person_t*, %struct.person_t** %4, align 8, !dbg !54
  %13 = call %struct.person_t* @f(%struct.person_t* %12), !dbg !55
  store %struct.person_t* %13, %struct.person_t** %5, align 8, !dbg !53
  call void @llvm.dbg.declare(metadata %struct.person_t** %6, metadata !56, metadata !DIExpression()), !dbg !57
  %14 = call %struct.person_t* @f(%struct.person_t* @p), !dbg !58
  store %struct.person_t* %14, %struct.person_t** %6, align 8, !dbg !57
  %15 = load %struct.person_t*, %struct.person_t** %5, align 8, !dbg !59
  %16 = getelementptr inbounds %struct.person_t, %struct.person_t* %15, i32 0, i32 0, !dbg !60
  %17 = load i32, i32* %16, align 8, !dbg !61
  %18 = add nsw i32 %17, 1, !dbg !61
  store i32 %18, i32* %16, align 8, !dbg !61
  %19 = load %struct.person_t*, %struct.person_t** %6, align 8, !dbg !62
  %20 = getelementptr inbounds %struct.person_t, %struct.person_t* %19, i32 0, i32 0, !dbg !63
  %21 = load i32, i32* %20, align 8, !dbg !64
  %22 = add nsw i32 %21, 1, !dbg !64
  store i32 %22, i32* %20, align 8, !dbg !64
  ret i32 0, !dbg !65
}

; Function Attrs: argmemonly nounwind willreturn
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* noalias nocapture writeonly, i8* noalias nocapture readonly, i64, i1 immarg) #2

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone speculatable willreturn }
attributes #2 = { argmemonly nounwind willreturn }

!llvm.dbg.cu = !{!2}
!llvm.module.flags = !{!25, !26, !27}
!llvm.ident = !{!28}

!0 = !DIGlobalVariableExpression(var: !1, expr: !DIExpression())
!1 = distinct !DIGlobalVariable(name: "clo", scope: !2, file: !3, line: 14, type: !20, isLocal: false, isDefinition: true)
!2 = distinct !DICompileUnit(language: DW_LANG_C99, file: !3, producer: "clang version 10.0.1 ", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !4, globals: !5, splitDebugInlining: false, nameTableKind: None)
!3 = !DIFile(filename: "person.c", directory: "/home/clf/pdg/Edge_Specification")
!4 = !{}
!5 = !{!0, !6}
!6 = !DIGlobalVariableExpression(var: !7, expr: !DIExpression())
!7 = distinct !DIGlobalVariable(name: "p", scope: !2, file: !3, line: 16, type: !8, isLocal: false, isDefinition: true)
!8 = !DIDerivedType(tag: DW_TAG_typedef, name: "Person", file: !3, line: 12, baseType: !9)
!9 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "person_t", file: !3, line: 8, size: 192, elements: !10)
!10 = !{!11, !13, !18}
!11 = !DIDerivedType(tag: DW_TAG_member, name: "age", scope: !9, file: !3, line: 9, baseType: !12, size: 32)
!12 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!13 = !DIDerivedType(tag: DW_TAG_member, name: "name", scope: !9, file: !3, line: 10, baseType: !14, size: 80, offset: 32)
!14 = !DICompositeType(tag: DW_TAG_array_type, baseType: !15, size: 80, elements: !16)
!15 = !DIBasicType(name: "char", size: 8, encoding: DW_ATE_signed_char)
!16 = !{!17}
!17 = !DISubrange(count: 10)
!18 = !DIDerivedType(tag: DW_TAG_member, name: "s", scope: !9, file: !3, line: 11, baseType: !19, size: 64, offset: 128)
!19 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !20, size: 64)
!20 = !DIDerivedType(tag: DW_TAG_typedef, name: "Clothes", file: !3, line: 6, baseType: !21)
!21 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "clothes", file: !3, line: 3, size: 128, elements: !22)
!22 = !{!23, !24}
!23 = !DIDerivedType(tag: DW_TAG_member, name: "color", scope: !21, file: !3, line: 4, baseType: !14, size: 80)
!24 = !DIDerivedType(tag: DW_TAG_member, name: "length", scope: !21, file: !3, line: 5, baseType: !12, size: 32, offset: 96)
!25 = !{i32 7, !"Dwarf Version", i32 4}
!26 = !{i32 2, !"Debug Info Version", i32 3}
!27 = !{i32 1, !"wchar_size", i32 4}
!28 = !{!"clang version 10.0.1 "}
!29 = distinct !DISubprogram(name: "f", scope: !3, file: !3, line: 18, type: !30, scopeLine: 18, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !4)
!30 = !DISubroutineType(types: !31)
!31 = !{!32, !32}
!32 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !8, size: 64)
!33 = !DILocalVariable(name: "p1", arg: 1, scope: !29, file: !3, line: 18, type: !32)
!34 = !DILocation(line: 18, column: 19, scope: !29)
!35 = !DILocation(line: 19, column: 5, scope: !29)
!36 = !DILocation(line: 19, column: 9, scope: !29)
!37 = !DILocation(line: 19, column: 12, scope: !29)
!38 = !DILocation(line: 20, column: 10, scope: !29)
!39 = !DILocation(line: 21, column: 12, scope: !29)
!40 = !DILocation(line: 21, column: 5, scope: !29)
!41 = distinct !DISubprogram(name: "main", scope: !3, file: !3, line: 24, type: !42, scopeLine: 24, spFlags: DISPFlagDefinition, unit: !2, retainedNodes: !4)
!42 = !DISubroutineType(types: !43)
!43 = !{!12}
!44 = !DILocalVariable(name: "c", scope: !41, file: !3, line: 25, type: !20)
!45 = !DILocation(line: 25, column: 13, scope: !41)
!46 = !DILocalVariable(name: "p1", scope: !41, file: !3, line: 26, type: !8)
!47 = !DILocation(line: 26, column: 12, scope: !41)
!48 = !DILocation(line: 26, column: 17, scope: !41)
!49 = !DILocation(line: 26, column: 22, scope: !41)
!50 = !DILocalVariable(name: "pt", scope: !41, file: !3, line: 27, type: !32)
!51 = !DILocation(line: 27, column: 13, scope: !41)
!52 = !DILocalVariable(name: "p3", scope: !41, file: !3, line: 28, type: !32)
!53 = !DILocation(line: 28, column: 13, scope: !41)
!54 = !DILocation(line: 28, column: 20, scope: !41)
!55 = !DILocation(line: 28, column: 18, scope: !41)
!56 = !DILocalVariable(name: "p4", scope: !41, file: !3, line: 29, type: !32)
!57 = !DILocation(line: 29, column: 13, scope: !41)
!58 = !DILocation(line: 29, column: 18, scope: !41)
!59 = !DILocation(line: 30, column: 5, scope: !41)
!60 = !DILocation(line: 30, column: 9, scope: !41)
!61 = !DILocation(line: 30, column: 12, scope: !41)
!62 = !DILocation(line: 31, column: 5, scope: !41)
!63 = !DILocation(line: 31, column: 9, scope: !41)
!64 = !DILocation(line: 31, column: 12, scope: !41)
!65 = !DILocation(line: 32, column: 5, scope: !41)
